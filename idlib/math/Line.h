#ifndef __LINE_H__
#define __LINE_H__

#include "Vector.h"

//returns (1/dx, 1/dy, 1/dz) vector for movement
idVec3 GetInverseMovementVelocity(const idVec3 &start, const idVec3 &end);

bool MovingBoundsIntersectBounds(
	//moving bounds: center for t = 0, velocity for t = [0..1], extent
	const idVec3 &startPosition, const idVec3 &invVelocity, const idVec3 &extent,
	//other bounds (standing still)
	const idBounds &objBounds,
	//in  [L,R]: time may vary as L <= t <= R    (usually L=0, R=1)
	//out [L,R]: common points exist during L <= t <= R
  float paramsRange[2]
);

#endif
