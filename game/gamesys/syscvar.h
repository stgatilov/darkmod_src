/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

#ifdef __linux__
#include "framework/cvarsystem.h"
#endif

/**
* DarkMod cvars - See text description in syscvar.cpp for descriptions
**/
extern idCVar cv_ai_sndvol;
extern idCVar cv_ai_bumpobject_impulse;
extern idCVar cv_ai_sight_prob;
extern idCVar cv_ai_sight_mag;
extern idCVar cv_ai_sightmaxdist;
extern idCVar cv_ai_sightmindist;
extern idCVar cv_ai_tactalert;
extern idCVar cv_ai_state_show;
extern idCVar cv_ai_task_show;
extern idCVar cv_ai_alertnum_show;
extern idCVar cv_ai_debug;
extern idCVar cv_ai_sight_thresh;
extern idCVar cv_ai_sight_scale;
extern idCVar cv_ai_show_enemy_visibility;
extern idCVar cv_ai_acuity_L1;
extern idCVar cv_ai_acuity_L2;
extern idCVar cv_ai_acuity_L3;
extern idCVar cv_ai_acuity_susp;
extern idCVar cv_ai_visdist_show;
extern idCVar cv_ai_opt_disable;
extern idCVar cv_ai_opt_noanims;
extern idCVar cv_ai_opt_novisualscan;
extern idCVar cv_ai_opt_forceopt;
extern idCVar cv_ai_opt_nothink;
extern idCVar cv_ai_opt_nomind;
extern idCVar cv_ai_opt_novisualstim;
extern idCVar cv_ai_opt_nolipsync;
extern idCVar cv_ai_opt_nopresent;

extern idCVar cv_sr_disable;

extern idCVar cv_spr_debug;
extern idCVar cv_spr_show;
extern idCVar cv_ko_show;
extern idCVar cv_ai_animstate_show;

extern idCVar cv_pm_runmod;
extern idCVar cv_pm_crouchmod;
extern idCVar cv_pm_creepmod;
extern idCVar cv_pm_pushmod;
extern idCVar cv_pm_mantle_height;
extern idCVar cv_pm_mantle_reach;
extern idCVar cv_pm_mantle_minflatness;
extern idCVar cv_pm_rope_snd_rep_dist;
extern idCVar cv_pm_rope_velocity_letgo;

/**
* This cvar controls if ai hiding spot search debug graphics are drawn
* If it is 0, then the graphics are not drawn.  If it is >= 1.0 then it
* is the number of milliseconds for which each graphic should persist.
* For example 3000.0 would mean 3 seconds
*/
extern idCVar cv_ai_search_show;

/**
* TDM Leaning vars:
**/
extern idCVar cv_pm_lean_angle;
extern idCVar cv_pm_lean_time;
extern idCVar cv_pm_lean_height;
extern idCVar cv_pm_lean_stretch;
extern idCVar cv_pm_lean_forward_angle;
extern idCVar cv_pm_lean_forward_time;
extern idCVar cv_pm_lean_forward_height;
extern idCVar cv_pm_lean_forward_stretch;
extern idCVar cv_pm_lean_to_valid_increments;
extern idCVar cv_pm_lean_door_increments;
extern idCVar cv_pm_lean_door_max;
extern idCVar cv_pm_lean_door_bounds_exp;
extern idCVar cv_pm_lean_toggle;

extern idCVar cv_frob_width;
extern idCVar cv_frob_debug_bounds;
extern idCVar cv_frob_fadetime;
extern idCVar cv_frob_ammo_selects_weapon;

// physics
extern idCVar cv_collision_damage_scale_vert;
extern idCVar cv_collision_damage_scale_horiz;
extern idCVar cv_collision_damage_min;
extern idCVar cv_drag_limit_force;
extern idCVar cv_drag_force_max;
extern idCVar cv_drag_stuck_dist;
extern idCVar cv_drag_damping;
extern idCVar cv_drag_damping_AF;
extern idCVar cv_drag_AF_ground_timer;
extern idCVar cv_drag_AF_free;
extern idCVar cv_drag_jump_masslimit;
extern idCVar cv_drag_encumber_minmass;
extern idCVar cv_drag_encumber_minmass;
extern idCVar cv_drag_encumber_maxmass;
extern idCVar cv_drag_encumber_max;

extern idCVar cv_throw_min;
extern idCVar cv_throw_max;
extern idCVar cv_throw_time;
extern idCVar cv_throw_max_vel;

extern idCVar cv_tdm_rope_pull_force_factor;

extern idCVar cv_tdm_hud_opacity;
extern idCVar cv_tdm_underwater_blur;

extern idCVar cv_tdm_inv_loot_group;
extern idCVar cv_tdm_inv_grouping;
extern idCVar cv_tdm_inv_groupvis;
extern idCVar cv_tdm_inv_hud_file;
extern idCVar cv_tdm_inv_loot_hud;
extern idCVar cv_tdm_inv_fadein;
extern idCVar cv_tdm_inv_fadeout;
extern idCVar cv_tdm_inv_loot_sound;

extern idCVar cv_pm_stepvol_walk;
extern idCVar cv_pm_stepvol_run;
extern idCVar cv_pm_stepvol_creep;
extern idCVar cv_pm_stepvol_crouch_walk;
extern idCVar cv_pm_stepvol_crouch_creep;
extern idCVar cv_pm_stepvol_crouch_run;

// Lightgem
extern idCVar cv_lg_distance;
extern idCVar cv_lg_xoffs;
extern idCVar cv_lg_yoffs;
extern idCVar cv_lg_zoffs;
extern idCVar cv_lg_oxoffs;
extern idCVar cv_lg_oyoffs;
extern idCVar cv_lg_ozoffs;
extern idCVar cv_lg_fov;
extern idCVar cv_lg_interleave;
extern idCVar cv_lg_hud;
extern idCVar cv_lg_weak;
extern idCVar cv_lg_player;
extern idCVar cv_lg_renderpasses;
extern idCVar cv_lg_debug;
extern idCVar cv_lg_model;
extern idCVar cv_lg_adjust;
extern idCVar cv_lg_split;
extern idCVar cv_lg_path;
extern idCVar cv_lg_crouch_modifier;
extern idCVar cv_lg_image_width;
extern idCVar cv_lg_screen_width;
extern idCVar cv_lg_screen_height;

// Lockpicking
extern idCVar cv_lp_pin_base_count;
extern idCVar cv_lp_sample_delay;
extern idCVar cv_lp_pick_timeout;
extern idCVar cv_lp_pick_attempts;
extern idCVar cv_lp_auto_pick;
extern idCVar cv_lp_randomize;
extern idCVar cv_lp_pawlow;

extern idCVar cv_dm_distance;

/**
* CVars added for Darkmod knockout and field of vision changes
*/
extern idCVar cv_ai_fov_show;
extern idCVar cv_ai_ko_show;


/**
* End DarkMod cvars
**/

extern idCVar	developer;

extern idCVar	g_cinematic;
extern idCVar	g_cinematicMaxSkipTime;

extern idCVar	r_aspectRatio;

extern idCVar	g_monsters;
extern idCVar	g_decals;
extern idCVar	g_knockback;
extern idCVar	g_skill;
extern idCVar	g_gravity;
extern idCVar	g_skipFX;
extern idCVar	g_skipParticles;
extern idCVar	g_bloodEffects;
extern idCVar	g_projectileLights;
extern idCVar	g_doubleVision;
extern idCVar	g_muzzleFlash;

extern idCVar	g_disasm;
extern idCVar	g_debugBounds;
extern idCVar	g_debugAnim;
extern idCVar	g_debugMove;
extern idCVar	g_debugDamage;
extern idCVar	g_debugWeapon;
extern idCVar	g_debugScript;
extern idCVar	g_debugMover;
extern idCVar	g_debugTriggers;
extern idCVar	g_debugCinematic;
extern idCVar	g_stopTime;
extern idCVar	g_armorProtection;
extern idCVar	g_armorProtectionMP;
extern idCVar	g_damageScale;
extern idCVar	g_useDynamicProtection;
extern idCVar	g_healthTakeTime;
extern idCVar	g_healthTakeAmt;
extern idCVar	g_healthTakeLimit;

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showCollisionTraces;
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showviewpos;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_showEnemies;

extern idCVar	g_frametime;
extern idCVar	g_timeentities;

extern idCVar	g_timeModifier;

extern idCVar	ai_debugScript;
extern idCVar	ai_debugMove;
extern idCVar	ai_debugTrajectory;
extern idCVar	ai_testPredictPath;
extern idCVar	ai_showCombatNodes;
extern idCVar	ai_showPaths;
extern idCVar	ai_showObstacleAvoidance;
extern idCVar	ai_blockedFailSafe;

extern idCVar	g_dvTime;
extern idCVar	g_dvAmplitude;
extern idCVar	g_dvFrequency;

extern idCVar	g_kickTime;
extern idCVar	g_kickAmplitude;
extern idCVar	g_blobTime;
extern idCVar	g_blobSize;

extern idCVar	g_testHealthVision;
extern idCVar	g_editEntityMode;
extern idCVar	g_dragEntity;
extern idCVar	g_dragDamping;
extern idCVar	g_dragShowSelection;
extern idCVar	g_dropItemRotation;

extern idCVar	g_vehicleVelocity;
extern idCVar	g_vehicleForce;
extern idCVar	g_vehicleSuspensionUp;
extern idCVar	g_vehicleSuspensionDown;
extern idCVar	g_vehicleSuspensionKCompress;
extern idCVar	g_vehicleSuspensionDamping;
extern idCVar	g_vehicleTireFriction;

extern idCVar	g_enablePortalSky;



extern idCVar	ik_enable;
extern idCVar	ik_debug;

extern idCVar	af_useLinearTime;
extern idCVar	af_useImpulseFriction;
extern idCVar	af_useJointImpulseFriction;
extern idCVar	af_useSymmetry;
extern idCVar	af_skipSelfCollision;
extern idCVar	af_skipLimits;
extern idCVar	af_skipFriction;
extern idCVar	af_forceFriction;
extern idCVar	af_maxLinearVelocity;
extern idCVar	af_maxAngularVelocity;
extern idCVar	af_timeScale;
extern idCVar	af_jointFrictionScale;
extern idCVar	af_contactFrictionScale;
extern idCVar	af_highlightBody;
extern idCVar	af_highlightConstraint;
extern idCVar	af_showTimings;
extern idCVar	af_showConstraints;
extern idCVar	af_showConstraintNames;
extern idCVar	af_showConstrainedBodies;
extern idCVar	af_showPrimaryOnly;
extern idCVar	af_showTrees;
extern idCVar	af_showLimits;
extern idCVar	af_showBodies;
extern idCVar	af_showBodyNames;
extern idCVar	af_showMass;
extern idCVar	af_showTotalMass;
extern idCVar	af_showInertia;
extern idCVar	af_showVelocity;
extern idCVar	af_showActive;
extern idCVar	af_testSolid;

extern idCVar	rb_showTimings;
extern idCVar	rb_showBodies;
extern idCVar	rb_showMass;
extern idCVar	rb_showInertia;
extern idCVar	rb_showVelocity;
extern idCVar	rb_showActive;

extern idCVar	pm_jumpheight;
extern idCVar	pm_stepsize;
//extern idCVar	pm_crouchspeed;
extern idCVar	pm_walkspeed;
//extern idCVar	pm_runspeed;
extern idCVar	pm_noclipspeed;
extern idCVar	pm_spectatespeed;
extern idCVar	pm_spectatebbox;
extern idCVar	pm_usecylinder;
extern idCVar	pm_minviewpitch;
extern idCVar	pm_maxviewpitch;
extern idCVar	pm_stamina;
extern idCVar	pm_staminathreshold;
extern idCVar	pm_staminarate;
extern idCVar	pm_crouchheight;
extern idCVar	pm_crouchviewheight;
extern idCVar	pm_normalheight;
extern idCVar	pm_normalviewheight;
extern idCVar	pm_deadheight;
extern idCVar	pm_deadviewheight;
extern idCVar	pm_crouchrate;
extern idCVar	pm_bboxwidth;
extern idCVar	pm_crouchbob;
extern idCVar	pm_walkbob;
extern idCVar	pm_runbob;
extern idCVar	pm_runpitch;
extern idCVar	pm_runroll;
extern idCVar	pm_bobup;
extern idCVar	pm_bobpitch;
extern idCVar	pm_bobroll;
extern idCVar	pm_thirdPersonRange;
extern idCVar	pm_thirdPersonHeight;
extern idCVar	pm_thirdPersonAngle;
extern idCVar	pm_thirdPersonClip;
extern idCVar	pm_thirdPerson;
extern idCVar	pm_thirdPersonDeath;
extern idCVar	pm_modelView;
extern idCVar	pm_airTics;

extern idCVar	g_showPlayerShadow;
extern idCVar	g_showHud;
extern idCVar	g_showProjectilePct;
extern idCVar	g_showBrass;
extern idCVar	g_gun_x;
extern idCVar	g_gun_y;
extern idCVar	g_gun_z;
extern idCVar	g_viewNodalX;
extern idCVar	g_viewNodalZ;
extern idCVar	g_fov;
extern idCVar	g_testDeath;
extern idCVar	g_skipViewEffects;
extern idCVar   g_mpWeaponAngleScale;

extern idCVar	g_testParticle;
extern idCVar	g_testParticleName;

extern idCVar	g_testPostProcess;
extern idCVar	g_rotoscope;

extern idCVar	g_testModelRotate;
extern idCVar	g_testModelAnimate;
extern idCVar	g_testModelBlend;
extern idCVar	g_exportMask;
extern idCVar	g_flushSave;

extern idCVar	aas_test;
extern idCVar	aas_showAreas;
extern idCVar	aas_showPath;
extern idCVar	aas_showFlyPath;
extern idCVar	aas_showWallEdges;
extern idCVar	aas_showHideArea;
extern idCVar	aas_pullPlayer;
extern idCVar	aas_randomPullPlayer;
extern idCVar	aas_goalArea;
extern idCVar	aas_showPushIntoArea;

extern idCVar	net_clientPredictGUI;

extern idCVar	g_voteFlags;
extern idCVar	g_mapCycle;
extern idCVar	g_balanceTDM;

extern idCVar	si_timeLimit;
extern idCVar	si_fragLimit;
extern idCVar	si_gameType;
extern idCVar	si_map;
extern idCVar	si_spectators;

extern idCVar	net_clientSelfSmoothing;

extern idCVar	net_clientLagOMeter;


extern const char *si_gameTypeArgs[];


extern const char *ui_skinArgs[];


#ifdef MOD_WATERPHYSICS

extern idCVar af_useBodyDensityBuoyancy;			// MOD_WATERPHYSICS

extern idCVar af_useFixedDensityBuoyancy;			// MOD_WATERPHYSICS

extern idCVar rb_showBuoyancy;								// MOD_WATERPHYSICS

#endif

// Bloom related - by JC_Denton & Maha_X - added by Dram
extern idCVar	r_bloom;
extern idCVar	r_bloom_blur_mult;
extern idCVar	r_bloom_src_mult;
extern idCVar   r_bloom_contrast_mult;				// clone_jc_denton
extern idCVar   r_bloom_contrast_min;				// clone_jc_denton
extern idCVar   r_bloom_shift_delay;				// clone_jc_denton
extern idCVar   r_bloom_blurIterations;				// clone_jc_denton
extern idCVar   r_bloom_buffer;						// clone_jc_denton
extern idCVar	r_bloom_hud;
extern idCVar	r_bloom_lightRayScale;

#endif /* !__SYS_CVAR_H__ */
