#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "tdm_shadowstencilsoft_shared.glsl"
#pragma tdm_include "tdm_poissondisk.glsl"  // note: adds uniform (only used if SSABSS_VOGEL is 0)

// Set to 0 if you prefer static sample pattern over per-pixel IGN rotation
#define SSABSS_ROTATE_SAMPLES 1
// 1 = golden-angle (Vogel) disk generated in the shader: any prefix of the samples is evenly spread,
// which allows to use fewer samples for small footprints (SSABSS_ADAPTIVE_SAMPLES)
// 0 = precomputed Poisson disk from u_softShadowsSamples (always uses all samples)
#define SSABSS_VOGEL 1
#define SSABSS_ADAPTIVE_SAMPLES SSABSS_VOGEL
#define SSABSS_MIN_SAMPLES 32
// penumbra when no blocker was found in the depth buffer (it may be off-screen or hidden), as fraction of the search distance:
// the result must not depend much on what the camera sees, so use a moderate value instead of the widest penumbra
#define SSABSS_NO_BLOCKER_DIST_FRAC 0.75
// Penumbra model, tuned to resemble the shadow map PCSS: blur = lightRadius * blockerToReceiver / lightToBlocker
// (the shadow map version limits it to 0.03 * lightRadius * lightDist, no saturation with distance)
// far field: penumbra of a blocker which is far from the receiver. Grows with the light distance, saturating slowly
#define SSABSS_FAR_SCALE 4.0
#define SSABSS_FAR_REF_DIST 90.0
// contact curve: >1 hardens shadows more close to the blocker (blur ~ pow(distance to blocker / search distance, power)), 1 = linear
#define SSABSS_CONTACT_POWER 1.6
// several overlapping casters: the shadow is computed with two penumbra widths, from the nearest blocker found (narrow)
// and from the farthest blocker found (wide), and the darker of the two wins. This way a close caster (apple) neither
// hardens nor brightens the penumbra of a farther one (wall). Needs SSABSS_VOGEL (narrow = the inner samples of the same disk).
// 1 = enabled, 0 = one penumbra width from the nearest blocker
#define SSABSS_TWO_PENUMBRAS 1
#define SSABSS_MIN_NARROW_SAMPLES 6
// brightness limits relative to the hard (unblurred) stencil at this pixel, as fraction of full light:
// a pixel which is inside some caster's shadow never gets brighter than SSABSS_SHADOWED_MAX_LIT (1.0 = no limit),
// a pixel which is lit by the stencil never gets darker than SSABSS_LIT_MIN_LIT (0.0 = no limit)
#define SSABSS_SHADOWED_MAX_LIT 0.6
#define SSABSS_LIT_MIN_LIT 0.0
// the tolerance of depth difference between receiver plane and sample grows with the blur radius (world units per world unit)
#define SSABSS_DEPTH_TOL_PER_RADIUS 0.03

float fetchStencilShadowTexture(usampler2D stencilTexture, vec2 texCoord) {
	float stTex = float(texture(stencilTexture, texCoord).r);
	return clamp(129.0 - stTex, 0.0, 1.0);
}

float interleavedGradientNoise(vec2 pixelPos) {
	vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
	return fract(magic.z * fract(dot(pixelPos, magic.xy)));
}

vec3 viewPosFromUvDepth(mat4 projectionMatrix, vec2 uv, float depth) {
	// depthToZ returns positive eye distance (-Z_view)
	float zEye = depthToZ(projectionMatrix, depth);
	vec2 ndc = uv * 2.0 - 1.0;
	return vec3(
		(ndc.x + projectionMatrix[2][0]) * zEye / projectionMatrix[0][0],
		(ndc.y + projectionMatrix[2][1]) * zEye / projectionMatrix[1][1],
		-zEye
	);
}

// Symmetric screen-space cone blocker search for contact hardening
// Returns (distance to the closest blocker, distance to the farthest blocker found), or (-1, -1) if no blocker was found.
// Only the surfaces visible in the depth buffer can be found, so "not found" does not mean "no occluder".
vec2 estimateContactBlockerDist(
	sampler2D depthTexture, mat4 projectionMatrix,
	vec3 P_v, vec3 N_v, vec3 L_dir, float rawLightDist,
	float lightRadius, float maxSearchDist, float noise, int steps,
	vec2 uvMin, vec2 uvMax
) {
	vec3 up = abs(L_dir.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 w1 = normalize(cross(L_dir, up));
	vec3 w2 = cross(L_dir, w1);

	float minBlockerDist = 1e9;
	float maxBlockerDist = 0.0;
	// Angular spread lets both shadowed pixels and adjacent outer-penumbra pixels detect contact blockers
	float coneTan = clamp(lightRadius / max(rawLightDist, 8.0), 0.04, 0.25);

	for (int i = 0; i < steps; i++) {
		float frac = (float(i) + noise) / float(steps);
		float t = maxSearchDist * frac * sqrt(frac); // frac^1.5: denser near contact seam

		float angle = 2.3999632 * float(i) + noise * 6.2831853;
		float spread = t * coneTan * (0.25 + 0.75 * frac);
		vec3 Q_v = P_v + t * L_dir + spread * (cos(angle) * w1 + sin(angle) * w2);

		vec4 clip = projectionMatrix * vec4(Q_v, 1.0);
		if (clip.w <= 0.0)
			continue;
		vec2 uv = (clip.xy / clip.w) * 0.5 + 0.5;
		if (any(lessThan(uv, uvMin)) || any(greaterThan(uv, uvMax)))
			continue;

		float sceneDepth = textureLod(depthTexture, uv, 0.0).r;
		vec3 B_v = viewPosFromUvDepth(projectionMatrix, uv, sceneDepth);

		float dz = (-Q_v.z) - (-B_v.z); // > 0 means ray stepped behind visible scene surface
		float bias = 0.012 * (-P_v.z) + 0.04 * t;
		vec3 diff = B_v - P_v;
		float dist3D = length(diff);

		if (dz > bias && dist3D < maxSearchDist) {
			// Require blocker to rise above the receiver's tangent plane (avoids curved self-hardening)
			// and to lie on the light's side of the receiver (a surface which is merely closer to the camera is no blocker)
			if (dot(N_v, diff) > 0.08 * dist3D && dot(L_dir, diff) > 0.5 * dist3D) {
				minBlockerDist = min(minBlockerDist, dist3D);
				maxBlockerDist = max(maxBlockerDist, dist3D);
			}
		}
	}

	return minBlockerDist < 1e8 ? vec2(minBlockerDist, maxBlockerDist) : vec2(-1.0);
}

// penumbra size factor (0..1, relative to the far field penumbra) for a blocker at the given distance from the receiver
float ssabssContactFactor(float d_occ, float maxSearchDist, float rawLightDist) {
	// PCSS contact ratio normalized against far-field fallback (smoothly blends to 1.0 at maxSearchDist)
	float blockerRatio = (d_occ / max(maxSearchDist, 1e-3))
		* (rawLightDist / max(rawLightDist - d_occ, 0.25 * rawLightDist));
	float k = pow(clamp(blockerRatio, 0.0, 1.0), SSABSS_CONTACT_POWER);
	return mix(k, 1.0, smoothstep(0.55 * maxSearchDist, maxSearchDist, d_occ));
}

float computeStencilSoftShadow(
	usampler2D stencilTexture, sampler2D depthTexture,
	vec3 objectToLight, vec3 objectNormal,
	mat4 modelViewMatrix, mat4 projectionMatrix,
	int softQuality, float softRadius,
	sampler2D stencilMipmapsTexture, ivec2 stencilMipmapsLevel, vec4 stencilMipmapsScissor
) {
	vec2 texSize = vec2(textureSize(stencilTexture, 0));
	vec2 pixSize = vec2(1.0, 1.0) / texSize;
	vec2 baseTexCoord = gl_FragCoord.xy * pixSize;

	// UV clamp bounds (keeps taps inside light scissor when mipmaps/scissor are active)
	vec2 uvMin = 0.5 * pixSize;
	vec2 uvMax = 1.0 - 0.5 * pixSize;
	if (stencilMipmapsLevel.x >= 0) {
		uvMin = max(uvMin, (stencilMipmapsScissor.xy + 0.5) * pixSize);
		uvMax = min(uvMax, (stencilMipmapsScissor.zw - 0.5) * pixSize);

		vec2 pixelCoords = clamp(gl_FragCoord.xy, stencilMipmapsScissor.xy, stencilMipmapsScissor.zw);
		vec2 mipmapsFullSize = textureSize(stencilMipmapsTexture, 0) * (1 << stencilMipmapsLevel.y);
		float mipmapValue = textureLod(
			stencilMipmapsTexture,
			pixelCoords / mipmapsFullSize,
			stencilMipmapsLevel.x - stencilMipmapsLevel.y
		).r;
		if (mipmapValue == 0.0)
			return 0.0;
		else if (mipmapValue == 1.0)
			return 1.0;
	}

	float stencil = fetchStencilShadowTexture(stencilTexture, baseTexCoord);
	float centerStencil = stencil;
	float sumWeight = 1.0;
	if (softQuality == 0) {
		return stencil;
	}

	float ign = interleavedGradientNoise(gl_FragCoord.xy);
	// decorrelated from the one used for rotation of the samples
	float ign2 = interleavedGradientNoise(gl_FragCoord.xy + vec2(5.588238, 5.588238));
	float rawLightDist = max(length(objectToLight), 1e-3);
	float lightRadius = softRadius;

	// 1. Bounded distance-based far penumbra (replaces unbounded lightDist / 66.6666)
	float r_far = lightRadius * SSABSS_FAR_SCALE * (rawLightDist / (rawLightDist + SSABSS_FAR_REF_DIST));

	// 2. View-space reconstruction & screen-space contact hardening
	vec3 P_v = viewPosFromUvDepth(projectionMatrix, baseTexCoord, gl_FragCoord.z);
	mat3 mv3 = mat3(modelViewMatrix);
	// interpolated vertex normal is not unit length
	vec3 N_v = normalize(mv3 * objectNormal);
	vec3 N_o = normalize(objectNormal);
	if (dot(N_v, -P_v) < 0.0) {
		// the surface is seen from behind: flip the normal everywhere in the same way
		N_v = -N_v;
		N_o = -N_o;
	}
	vec3 L_dir = normalize(mv3 * objectToLight);

	const float MAX_CONTACT_DIST = 52.0; // ~1.3m contact hardening zone
	float maxSearchDist = min(MAX_CONTACT_DIST, rawLightDist * 0.75);
	int blockerSteps = clamp(softQuality / 3, 4, 8);

	vec2 occ = estimateContactBlockerDist(
		depthTexture, projectionMatrix,
		P_v, N_v, L_dir, rawLightDist,
		lightRadius, maxSearchDist, ign2, blockerSteps,
		uvMin, uvMax
	);
	float dNear = occ.x;
	float dFar = occ.y;
	if (occ.x < 0.0) {
		// the occluder may be invisible for the camera (off-screen, hidden): don't make the penumbra depend on that too much
		dNear = dFar = SSABSS_NO_BLOCKER_DIST_FRAC * maxSearchDist;
	}
#if SSABSS_TWO_PENUMBRAS && SSABSS_VOGEL
	float kNear = ssabssContactFactor(dNear, maxSearchDist, rawLightDist);
	float kFar = ssabssContactFactor(dFar, maxSearchDist, rawLightDist);
	float k_contact = max(kFar, kNear);
	// the narrow penumbra is the same disk with smaller radius
	float narrowRatio = clamp(kNear / max(k_contact, 1e-4), 0.0, 1.0);
#else
	float k_contact = ssabssContactFactor(dNear, maxSearchDist, rawLightDist);
#endif
	float blurRadiusWorld = r_far * k_contact;

	// 3. SSABSS tangent-plane basis + light-grazing elongation in object space
	vec3 objL = objectToLight / rawLightDist;
	vec3 perpL = objectToLight - dot(objectToLight, N_o) * N_o;
	vec3 alongDirW = (dot(perpL, perpL) > 1e-6)
		? normalize(perpL)
		: normalize(cross(N_o, abs(N_o.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
	vec3 orthoDirW = cross(N_o, alongDirW);

	float NdotL = dot(objL, N_o);
	alongDirW *= blurRadiusWorld / max(NdotL, 0.25);
	orthoDirW *= blurRadiusWorld;

	// Convert both vectors into screen UV space via exact clip-space derivative
	mat4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
	vec2 alongDir = (mat3(modelViewProjectionMatrix) * alongDirW).xy;
	vec2 orthoDir = (mat3(modelViewProjectionMatrix) * orthoDirW).xy;
	vec3 mvpRow3 = vec3(modelViewProjectionMatrix[0][3], modelViewProjectionMatrix[1][3], modelViewProjectionMatrix[2][3]);
	float along_w = dot(mvpRow3, alongDirW);
	float ortho_w = dot(mvpRow3, orthoDirW);

	vec2 thisNdc = 2.0 * baseTexCoord - vec2(1.0);
	alongDir -= thisNdc * along_w;
	orthoDir -= thisNdc * ortho_w;
	alongDir *= gl_FragCoord.w * 0.5;
	orthoDir *= gl_FragCoord.w * 0.5;

	// 4. Enforce mipmap upper bound on each axis separately (the elongated axis must not shrink the other one)
	// and a minimum 0.75px AA floor on each axis
	float lenX = length(alongDir * texSize);
	float lenY = length(orthoDir * texSize);
	float maxBlurAxisLength = computeMaxBlurAxisLength(texSize.y, softQuality);

	float scaleAlong = min(1.0, maxBlurAxisLength / max(lenX, 1e-5));
	float scaleOrtho = min(1.0, maxBlurAxisLength / max(lenY, 1e-5));
	alongDir *= scaleAlong;
	alongDirW *= scaleAlong;
	lenX *= scaleAlong;
	orthoDir *= scaleOrtho;
	orthoDirW *= scaleOrtho;
	lenY *= scaleOrtho;

	const float MIN_AXIS_PX = 0.75;
	if (lenX > 1e-5 && lenX < MIN_AXIS_PX) {
		alongDir *= MIN_AXIS_PX / lenX;
		lenX = MIN_AXIS_PX;
	} else if (lenX <= 1e-5) {
		alongDir = vec2(MIN_AXIS_PX, 0.0) * pixSize;
		lenX = MIN_AXIS_PX;
	}
	if (lenY > 1e-5 && lenY < MIN_AXIS_PX) {
		orthoDir *= MIN_AXIS_PX / lenY;
		lenY = MIN_AXIS_PX;
	} else if (lenY <= 1e-5) {
		orthoDir = vec2(0.0, MIN_AXIS_PX) * pixSize;
		lenY = MIN_AXIS_PX;
	}

	// View-space Z deltas of the tangent vectors (for silhouette-safe analytic plane depth check)
	float alongViewZ = (mv3 * alongDirW).z;
	float orthoViewZ = (mv3 * orthoDirW).z;

	float Z00 = -P_v.z;
	vec2 tanFovHalf = vec2(1.0 / projectionMatrix[0][0], 1.0 / projectionMatrix[1][1]);
	vec2 canonDerivs = 2.0 * Z00 * tanFovHalf;
	float depthTolFloor = 0.5 + SSABSS_DEPTH_TOL_PER_RADIUS * blurRadiusWorld;

	// 5. Number of samples: the footprint is an ellipse, sample density is one per avgSampleDist^2 pixels
	int numSamples = softQuality;
#if SSABSS_ADAPTIVE_SAMPLES
	float avgSampleDist = computeMaxBlurAxisLength(texSize.y, 1.0);
	int neededSamples = int(ceil(lenX * lenY / (avgSampleDist * avgSampleDist)));
	numSamples = min(softQuality, max(SSABSS_MIN_SAMPLES, neededSamples));
#endif

	float rotAngle = 0.0;
#if SSABSS_ROTATE_SAMPLES
	rotAngle = ign * 6.2831853;
#endif
#if !SSABSS_VOGEL
	float cRot = cos(rotAngle);
	float sRot = sin(rotAngle);
#endif

#if SSABSS_TWO_PENUMBRAS && SSABSS_VOGEL
	// Vogel disk is ordered by radius: the first samples cover the inner disk. Area ratio = narrowRatio^2
	int numNarrow = numSamples;
	if (narrowRatio < 0.999)
		numNarrow = clamp(int(ceil(float(numSamples) * narrowRatio * narrowRatio)), min(SSABSS_MIN_NARROW_SAMPLES, numSamples), numSamples);
	float stencilNarrow = stencil;
	float sumWeightNarrow = 1.0;
#endif

	for (int i = 0; i < numSamples; i++) {
#if SSABSS_VOGEL
		float sampleRadius = sqrt((float(i) + 0.5) / float(numSamples));
		float sampleAngle = float(i) * 2.39996323 + rotAngle;
		vec2 s = sampleRadius * vec2(cos(sampleAngle), sin(sampleAngle));
#else
		vec2 s = u_softShadowsSamples[i];
		s = vec2(cRot * s.x - sRot * s.y, sRot * s.x + cRot * s.y);
#endif
		vec2 delta = s.x * alongDir + s.y * orthoDir;
		vec2 sampleTexCoord = clamp(baseTexCoord + delta, uvMin, uvMax);

		// Analytic tangent-plane depth change (avoids dFdx/dFdy spikes at silhouette edges)
		float Zdiff = depthToZ(projectionMatrix, textureLod(depthTexture, sampleTexCoord, 0.0).r) - Z00;
		float tangentZdiff = -(s.x * alongViewZ + s.y * orthoViewZ);
		float deg45diff = dot(canonDerivs, abs(delta));
		float ZdiffTol = max(abs(tangentZdiff) * 0.4 + deg45diff * 0.18, depthTolFloor);

		float err = max(abs(Zdiff - tangentZdiff) - ZdiffTol * 0.5, 0.0);
		float q = err / ZdiffTol;
		float weight = exp(-q * q);

		float st = fetchStencilShadowTexture(stencilTexture, sampleTexCoord) * weight;
		stencil += st;
		sumWeight += weight;
#if SSABSS_TWO_PENUMBRAS && SSABSS_VOGEL
		if (i < numNarrow) {
			stencilNarrow += st;
			sumWeightNarrow += weight;
		}
#endif
	}

	float result = stencil / sumWeight;
#if SSABSS_TWO_PENUMBRAS && SSABSS_VOGEL
	// darker of the two penumbras
	result = min(result, stencilNarrow / sumWeightNarrow);
#endif
	// the stencil is a union of all casters, so this limits light leaking into any caster's shadow from blurring
	if (centerStencil < 0.5)
		result = min(result, SSABSS_SHADOWED_MAX_LIT);
	else
		result = max(result, SSABSS_LIT_MIN_LIT);
	return result;
}
