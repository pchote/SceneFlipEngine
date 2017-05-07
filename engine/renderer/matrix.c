/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 * Based on matrixUtil.c, Copyright (C) 2010~2011 Apple Inc. All Rights Reserved.
 */

// TODO: Add proper apple licence header or rewrite code

#include "matrix.h"
#include <math.h>
#include <string.h>

void mtxLoadIdentity(float* mtx)
{
	// [ 0 4  8 12 ]
	// [ 1 5  9 13 ]
	// [ 2 6 10 14 ]
    // [ 3 7 11 15 ]
	mtx[ 0] = mtx[ 5] = mtx[10] = mtx[15] = 1.0f;
	
	mtx[ 1] = mtx[ 2] = mtx[ 3] = mtx[ 4] =    
	mtx[ 6] = mtx[ 7] = mtx[ 8] = mtx[ 9] =    
	mtx[11] = mtx[12] = mtx[13] = mtx[14] = 0.0;
}

// Note: this differs from the usual definition by having width map from -1 -> 1
// and height relative by the aspect ratio, instead of the opposite.
void mtxLoadPerspective(float* mtx, float fov, float aspect, float nearZ, float farZ)
{
	float f = 1.0f / tanf( (fov * (M_PI/180)) / 2.0f);
	
	mtx[0] = f;
	mtx[1] = 0.0f;
	mtx[2] = 0.0f;
	mtx[3] = 0.0f;
	
	mtx[4] = 0.0f;
	mtx[5] = f * aspect;
	mtx[6] = 0.0f;
	mtx[7] = 0.0f;
	
	mtx[8] = 0.0f;
	mtx[9] = 0.0f;
	mtx[10] = (farZ+nearZ) / (nearZ-farZ);
	mtx[11] = -1.0f;
	
	mtx[12] = 0.0f;
	mtx[13] = 0.0f;
	mtx[14] = 2 * farZ * nearZ /  (nearZ-farZ);
	mtx[15] = 0.0f;
}


void mtxLoadOrthographic(float* mtx,
							float left, float right, 
							float bottom, float top, 
							float nearZ, float farZ)
{
	//See appendix G of OpenGL Red Book
	
	mtx[ 0] = 2.0f / (right - left);
	mtx[ 1] = 0.0;
	mtx[ 2] = 0.0;
	mtx[ 3] = 0.0;
	
	mtx[ 4] = 0.0;
	mtx[ 5] = 2.0f / (top - bottom);
	mtx[ 6] = 0.0;
	mtx[ 7] = 0.0;
	
	mtx[ 8] = 0.0;
	mtx[ 9] = 0.0;
	mtx[10] = -2.0f / (farZ - nearZ);
	mtx[11] = 0.0;
	
	mtx[12] = -(right + left) / (right - left);
	mtx[13] = -(top + bottom) / (top - bottom);
	mtx[14] = -(farZ + nearZ) / (farZ - nearZ);
	mtx[15] = 1.0f;
}

void mtxMultiply(float* ret, const float* lhs, const float* rhs)
{
	// [ 0 4  8 12 ]   [ 0 4  8 12 ]
	// [ 1 5  9 13 ] x [ 1 5  9 13 ]
	// [ 2 6 10 14 ]   [ 2 6 10 14 ]
	// [ 3 7 11 15 ]   [ 3 7 11 15 ]
	ret[ 0] = lhs[ 0]*rhs[ 0] + lhs[ 4]*rhs[ 1] + lhs[ 8]*rhs[ 2] + lhs[12]*rhs[ 3];
	ret[ 1] = lhs[ 1]*rhs[ 0] + lhs[ 5]*rhs[ 1] + lhs[ 9]*rhs[ 2] + lhs[13]*rhs[ 3];
	ret[ 2] = lhs[ 2]*rhs[ 0] + lhs[ 6]*rhs[ 1] + lhs[10]*rhs[ 2] + lhs[14]*rhs[ 3];
	ret[ 3] = lhs[ 3]*rhs[ 0] + lhs[ 7]*rhs[ 1] + lhs[11]*rhs[ 2] + lhs[15]*rhs[ 3];
    
	ret[ 4] = lhs[ 0]*rhs[ 4] + lhs[ 4]*rhs[ 5] + lhs[ 8]*rhs[ 6] + lhs[12]*rhs[ 7];
	ret[ 5] = lhs[ 1]*rhs[ 4] + lhs[ 5]*rhs[ 5] + lhs[ 9]*rhs[ 6] + lhs[13]*rhs[ 7];
	ret[ 6] = lhs[ 2]*rhs[ 4] + lhs[ 6]*rhs[ 5] + lhs[10]*rhs[ 6] + lhs[14]*rhs[ 7];
	ret[ 7] = lhs[ 3]*rhs[ 4] + lhs[ 7]*rhs[ 5] + lhs[11]*rhs[ 6] + lhs[15]*rhs[ 7];
    
	ret[ 8] = lhs[ 0]*rhs[ 8] + lhs[ 4]*rhs[ 9] + lhs[ 8]*rhs[10] + lhs[12]*rhs[11];
	ret[ 9] = lhs[ 1]*rhs[ 8] + lhs[ 5]*rhs[ 9] + lhs[ 9]*rhs[10] + lhs[13]*rhs[11];
	ret[10] = lhs[ 2]*rhs[ 8] + lhs[ 6]*rhs[ 9] + lhs[10]*rhs[10] + lhs[14]*rhs[11];
	ret[11] = lhs[ 3]*rhs[ 8] + lhs[ 7]*rhs[ 9] + lhs[11]*rhs[10] + lhs[15]*rhs[11];
    
	ret[12] = lhs[ 0]*rhs[12] + lhs[ 4]*rhs[13] + lhs[ 8]*rhs[14] + lhs[12]*rhs[15];
	ret[13] = lhs[ 1]*rhs[12] + lhs[ 5]*rhs[13] + lhs[ 9]*rhs[14] + lhs[13]*rhs[15];
	ret[14] = lhs[ 2]*rhs[12] + lhs[ 6]*rhs[13] + lhs[10]*rhs[14] + lhs[14]*rhs[15];
	ret[15] = lhs[ 3]*rhs[12] + lhs[ 7]*rhs[13] + lhs[11]*rhs[14] + lhs[15]*rhs[15];
}

void mtxMultiplyVec3(float ret[3], const float *mtx, const float vec[3])
{
    float x = mtx[0]*vec[0] + mtx[4]*vec[1] + mtx[8]*vec[2] + mtx[12];
    float y = mtx[1]*vec[0] + mtx[5]*vec[1] + mtx[9]*vec[2] + mtx[13];
    float z = mtx[2]*vec[0] + mtx[6]*vec[1] + mtx[10]*vec[2] + mtx[14];
    float w = mtx[3]*vec[0] + mtx[7]*vec[1] + mtx[11]*vec[2] + mtx[15];
    ret[0] = x/w;
    ret[1] = y/w;
    ret[2] = z/w;
}

void mtxTranslateApply(float* mtx, float xTrans, float yTrans, float zTrans)
{
	// [ 0 4  8 12 ]   [ 1 0 0 x ]
	// [ 1 5  9 13 ] x [ 0 1 0 y ]
	// [ 2 6 10 14 ]   [ 0 0 1 z ]
	// [ 3 7 11 15 ]   [ 0 0 0 1 ]
	
	mtx[12] += mtx[0]*xTrans + mtx[4]*yTrans + mtx[ 8]*zTrans;
	mtx[13] += mtx[1]*xTrans + mtx[5]*yTrans + mtx[ 9]*zTrans;
	mtx[14] += mtx[2]*xTrans + mtx[6]*yTrans + mtx[10]*zTrans;	
}

void mtxScaleApply(float* mtx, float xScale, float yScale, float zScale)
{ 
    // [ 0 4  8 12 ]   [ x 0 0 0 ]
    // [ 1 5  9 13 ] x [ 0 y 0 0 ] 
    // [ 2 6 10 14 ]   [ 0 0 z 0 ]
    // [ 3 7 11 15 ]   [ 0 0 0 1 ]   
	
	mtx[ 0] *= xScale;
	mtx[ 4] *= yScale;
	mtx[ 8] *= zScale;
	
	mtx[ 1] *= xScale;
	mtx[ 5] *= yScale;
	mtx[ 9] *= zScale;
	
	mtx[ 2] *= xScale;
	mtx[ 6] *= yScale;
	mtx[10] *= zScale;
	
	mtx[ 3] *= xScale;
	mtx[ 7] *= yScale;
	mtx[11] *= xScale;
}

void mtxRotateXApply(float* mtx, float deg)
{
	// [ 0 4  8 12 ]   [ 1  0    0  0 ]
	// [ 1 5  9 13 ] x [ 0 cos -sin 0 ]
	// [ 2 6 10 14 ]   [ 0 sin  cos 0 ]
	// [ 3 7 11 15 ]   [ 0  0    0  1 ]
	
	float rad = deg * (M_PI/180.0f);
	
	float cosrad = cosf(rad);
	float sinrad = sinf(rad);
	
	float mtx04 = mtx[4];
	float mtx05 = mtx[5];
	float mtx06 = mtx[6];
	float mtx07 = mtx[7];
	
	mtx[ 4] = mtx[ 8]*sinrad + mtx04*cosrad;
	mtx[ 8] = mtx[ 8]*cosrad - mtx04*sinrad;
	
	mtx[ 5] = mtx[ 9]*sinrad + mtx05*cosrad;
	mtx[ 9] = mtx[ 9]*cosrad - mtx05*sinrad;
	
	mtx[ 6] = mtx[10]*sinrad + mtx06*cosrad;
	mtx[10] = mtx[10]*cosrad - mtx06*sinrad;
	
	mtx[ 7] = mtx[11]*sinrad + mtx07*cosrad;
	mtx[11] = mtx[11]*cosrad - mtx07*sinrad;
}

void mtxRotateYApply(float* mtx, float deg)
{
	// [ 0 4  8 12 ]   [ cos 0  -sin 0 ]
	// [ 1 5  9 13 ] x [ 0   1  0    0 ]
	// [ 2 6 10 14 ]   [ sin 0  cos  0 ]
	// [ 3 7 11 15 ]   [ 0   0  0    1 ]
	
	float rad = deg * (M_PI/180.0f);
	
	float cosrad = cosf(rad);
	float sinrad = sinf(rad);
	
	float mtx00 = mtx[0];
	float mtx01 = mtx[1];
	float mtx02 = mtx[2];
	float mtx03 = mtx[3];
	
	mtx[ 0] = mtx[ 8]*sinrad + mtx00*cosrad;
	mtx[ 8] = mtx[ 8]*cosrad - mtx00*sinrad;
	
	mtx[ 1] = mtx[ 9]*sinrad + mtx01*cosrad;
	mtx[ 9] = mtx[ 9]*cosrad - mtx01*sinrad;
	
	mtx[ 2] = mtx[10]*sinrad + mtx02*cosrad;
	mtx[10] = mtx[10]*cosrad - mtx02*sinrad;
	
	mtx[ 3] = mtx[11]*sinrad + mtx03*cosrad;
	mtx[11] = mtx[11]*cosrad - mtx03*sinrad;
}

void mtxRotateZApply(float* mtx, float deg)
{
	// [ 0 4  8 12 ]   [ cos -sin 0  0 ]
	// [ 1 5  9 13 ] x [ sin cos  0  0 ]
	// [ 2 6 10 14 ]   [ 0   0    1  0 ]
	// [ 3 7 11 15 ]   [ 0   0    0  1 ]
	
	float rad = deg * (M_PI/180.0f);
	
	float cosrad = cosf(rad);
	float sinrad = sinf(rad);
	
	float mtx00 = mtx[0];
	float mtx01 = mtx[1];
	float mtx02 = mtx[2];
	float mtx03 = mtx[3];
	
	mtx[ 0] = mtx[ 4]*sinrad + mtx00*cosrad;
	mtx[ 4] = mtx[ 4]*cosrad - mtx00*sinrad;
	
	mtx[ 1] = mtx[ 5]*sinrad + mtx01*cosrad;
	mtx[ 5] = mtx[ 5]*cosrad - mtx01*sinrad;
	
	mtx[ 2] = mtx[ 6]*sinrad + mtx02*cosrad;
	mtx[ 6] = mtx[ 6]*cosrad - mtx02*sinrad;
	
	mtx[ 3] = mtx[ 7]*sinrad + mtx03*cosrad;
	mtx[ 7] = mtx[ 7]*cosrad - mtx03*sinrad;
}

void mtxRotateApply(float* mtx, float deg, float xAxis, float yAxis, float zAxis)
{	
	if(yAxis == 0.0f && zAxis == 0.0f)
	{
		mtxRotateXApply(mtx, deg);
	}
	else if(xAxis == 0.0f && zAxis == 0.0f)
	{
		mtxRotateYApply(mtx, deg);
	}
	else if(xAxis == 0.0f && yAxis == 0.0f)
	{
		mtxRotateZApply(mtx, deg);
	}
	else
	{
		float rad = deg * M_PI/180.0f;
		
		float sin_a = sinf(rad);
		float cos_a = cosf(rad);
		
		// Calculate coeffs.  No need to check for zero magnitude because we wouldn't be here.
		float magnitude = sqrtf(xAxis * xAxis + yAxis * yAxis + zAxis * zAxis);
		
		float p = 1.0f / magnitude;
		float cos_am = 1.0f - cos_a;
		
		float xp = xAxis * p;
		float yp = yAxis * p;
		float zp = zAxis * p;
		
		float xx = xp * xp;
		float yy = yp * yp;
		float zz = zp * zp;
		
		float xy = xp * yp * cos_am;
		float yz = yp * zp * cos_am;
		float zx = zp * xp * cos_am;
		
		xp *= sin_a;
		yp *= sin_a;
		zp *= sin_a;
		
		// Load coefs
		float m0  = xx + cos_a * (1.0f - xx);
		float m1  = xy + zp;
		float m2  = zx - yp;
		float m4  = xy - zp;
		float m5  = yy + cos_a * (1.0f - yy);
		float m6  = yz + xp;
		float m8  = zx + yp;
		float m9  = yz - xp;
		float m10 = zz + cos_a * (1.0f - zz);
		
		// Apply rotation 
		float c1 = mtx[0];
		float c2 = mtx[4];
		float c3 = mtx[8];
		mtx[0]  = c1 * m0 + c2 * m1 + c3 * m2;
		mtx[4]  = c1 * m4 + c2 * m5 + c3 * m6;
		mtx[8]  = c1 * m8 + c2 * m9 + c3 * m10;
		
		c1 = mtx[1];
		c2 = mtx[5];
		c3 = mtx[9];
		mtx[1]  = c1 * m0 + c2 * m1 + c3 * m2;
		mtx[5]  = c1 * m4 + c2 * m5 + c3 * m6;
		mtx[9]  = c1 * m8 + c2 * m9 + c3 * m10;
		
		c1 = mtx[2];
		c2 = mtx[6];
		c3 = mtx[10];
		mtx[2]  = c1 * m0 + c2 * m1 + c3 * m2;
		mtx[6]  = c1 * m4 + c2 * m5 + c3 * m6;
		mtx[10] = c1 * m8 + c2 * m9 + c3 * m10;
		
		c1 = mtx[3];
		c2 = mtx[7];
		c3 = mtx[11];
		mtx[3]  = c1 * m0 + c2 * m1 + c3 * m2;
		mtx[7]  = c1 * m4 + c2 * m5 + c3 * m6;
		mtx[11] = c1 * m8 + c2 * m9 + c3 * m10;
	}	
}
