/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 * Based on matrixUtil.h, Copyright (C) 2010~2011 Apple Inc. All Rights Reserved.
 */

#ifndef GPEngine_matrix_h
#define GPEngine_matrix_h

void mtxLoadIdentity(float* mtx);
void mtxLoadPerspective(float* mtx, float fov, float aspect, float nearZ, float farZ);
void mtxLoadOrthographic(float* mtx,
								float left, float right,
								float bottom, float top,
								float nearZ, float farZ);

void mtxMultiply(float* ret, const float* lhs, const float* rhs);
void mtxMultiplyVec3(float *ret, const float *mtx, const float *vec);
void mtxTranslateApply(float* mtx, float xTrans, float yTrans, float zTrans);
void mtxScaleApply(float* mtx, float xScale, float yScale, float zScale);
void mtxRotateApply(float* mtx, float deg, float xAxis, float yAxis, float zAxis);
void mtxRotateXApply(float* mtx, float rad);
void mtxRotateYApply(float* mtx, float rad);
void mtxRotateZApply(float* mtx, float rad);

#endif
