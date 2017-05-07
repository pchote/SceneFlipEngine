/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 *
 * SceneFlipEngine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SceneFlipEngine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SceneFlipEngine.  If not, see <http://www.gnu.org/licenses/>.
 */

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#include "engine.h"

typedef enum
{
    DIRECTION_LEFT  = (1 << 0),
    DIRECTION_RIGHT = (1 << 1),
    DIRECTION_UP    = (1 << 2),
    DIRECTION_DOWN  = (1 << 3),
} analogDirectionFlags;

@interface GPMacGLView : NSOpenGLView
{
    CVDisplayLinkRef displayLink;
    engine_ptr gameEngine;
    uint64_t lastTick;
}

@property uint8_t directionKeyFlags;
@property uint8_t cameraKeyFlags;

@end
