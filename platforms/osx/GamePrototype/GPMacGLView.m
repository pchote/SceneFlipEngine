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

#import "GPMacGLView.h"
#import <QuartzCore/CVHostTime.h>

#include "engine.h"

@implementation GPMacGLView

@synthesize directionKeyFlags = _directionKeyFlags;
@synthesize cameraKeyFlags = _cameraKeyFlags;

- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
	// There is no autorelease pool when this method is called
	// because it will be called from a background thread
	// It's important to create one or you will leak objects
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    // Ugly, but works... for now
    // TODO: Look into doing rendering in a separate thread to ticking
    double dt = (outputTime->videoTime - lastTick)*1.0/outputTime->videoTimeScale;

    lastTick = outputTime->videoTime;
    [[self openGLContext] makeCurrentContext];

	// We draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main thread
	// Add a mutex around to avoid the threads accessing the context simultaneously	when resizing
	CGLLockContext([[self openGLContext] CGLContextObj]);
    engine_tick(gameEngine, dt);
    engine_draw(gameEngine);
	CGLFlushDrawable([[self openGLContext] CGLContextObj]);
	CGLUnlockContext([[self openGLContext] CGLContextObj]);

	[pool release];
	return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now,
    const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CVReturn result = [(GPMacGLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (void)awakeFromNib
{
    NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
		0
	};

	NSOpenGLPixelFormat *pf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs] autorelease];

	if (!pf)
		NSLog(@"No OpenGL pixel format");

    NSOpenGLContext* context = [[[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil] autorelease];

    [self setPixelFormat:pf];
    [self setOpenGLContext:context];
}

- (void)prepareOpenGL
{
	[super prepareOpenGL];

	// Make all the OpenGL calls to setup rendering
	//  and build the necessary rendering objects
	[self initGL];

	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, self);

	// Set the display link for the current renderer
	CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

	// Activate the display link
	CVDisplayLinkStart(displayLink);
}

- (void) initGL
{
	// Make this openGL context current to the thread
	// (i.e. all openGL on this thread calls will go to this context)
	[[self openGLContext] makeCurrentContext];

	// Synchronize buffer swaps with vertical refresh rate
	[[self openGLContext] setValues:&(GLint){1} forParameter:NSOpenGLCPSwapInterval];

    NSRect rect = [self bounds];
    gameEngine = engine_create([[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"assets"] UTF8String], rect.size.width, rect.size.height);
    lastTick = CVGetCurrentHostTime();
}

- (void)reshape
{
	[super reshape];

	// We draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main thread
	// Add a mutex around to avoid the threads accessing the context simultaneously when resizing
	CGLLockContext([[self openGLContext] CGLContextObj]);

	NSRect rect = [self bounds];
	engine_set_viewport(gameEngine, rect.size.width, rect.size.height);

	CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)dealloc
{
	// Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been release
	CVDisplayLinkStop(displayLink);
	CVDisplayLinkRelease(displayLink);

	// Release the display link AFTER display link has been released
    engine_destroy(gameEngine);

	[super dealloc];
}

#pragma mark Keyboard Handling
-(BOOL)acceptsFirstResponder { return YES; }
-(BOOL)becomeFirstResponder { return YES; }
-(BOOL)resignFirstResponder { return YES; }

-(GPpolar)analogInputForFlags: (uint8_t)flags
{
    int8_t dx = 0;
    int8_t dy = 0;

    if (flags & DIRECTION_UP)
        dy++;
    if (flags & DIRECTION_DOWN)
        dy--;
    if (flags & DIRECTION_RIGHT)
        dx++;
    if (flags & DIRECTION_LEFT)
        dx--;

    if (dx == 0 && dy == 0)
        return (GPpolar){0, 0};
    return (GPpolar){1, atan2(dy, dx)};
}

-(void)updateAnalogInputsForEvent:(NSEvent *)theEvent keyDown:(BOOL)down
{
    NSString *str = [theEvent charactersIgnoringModifiers];
    NSUInteger len = [str length];
    const char *cstr = [[str lowercaseString] UTF8String];
    analogDirectionFlags dirflags = 0;
    analogDirectionFlags camflags = 0;

    for (NSUInteger i = 0; i < len; i++)
    {
        switch (cstr[i])
        {
            case 'w': dirflags |= DIRECTION_UP; break;
            case 'a': dirflags |= DIRECTION_LEFT; break;
            case 's': dirflags |= DIRECTION_DOWN; break;
            case 'd': dirflags |= DIRECTION_RIGHT; break;
            case 'i': camflags |= DIRECTION_UP; break;
            case 'j': camflags |= DIRECTION_LEFT; break;
            case 'k': camflags |= DIRECTION_DOWN; break;
            case 'l': camflags |= DIRECTION_RIGHT; break;
        }
    }

    if (down)
    {
        self.directionKeyFlags |= dirflags;
        self.cameraKeyFlags |= camflags;
    }
    else
    {
        self.directionKeyFlags &= ~dirflags;
        self.cameraKeyFlags &= ~camflags;
    }

    engine_set_analog_input(gameEngine, ANALOG_INPUT_DIRECTION, [self analogInputForFlags:self.directionKeyFlags]);
    engine_set_analog_input(gameEngine, ANALOG_INPUT_CAMERA, [self analogInputForFlags:self.cameraKeyFlags]);
}

-(void)updateDiscreteInputsForEvent:(NSEvent *)theEvent keyDown:(BOOL)down
{
    engine_config_ptr config = engine_get_config_ref(gameEngine);
    NSString *str = [theEvent charactersIgnoringModifiers];
    NSUInteger len = [str length];
    const char *cstr = [[str lowercaseString] UTF8String];
    input_flags flags = 0;

    for (NSUInteger i = 0; i < len; i++)
    {
        switch (cstr[i])
        {
            case 'o':
                if (down)
                    config->debug_render_layer_mesh ^= true;
                    engine_update_overlay_display(gameEngine);
                break;
            case 'p':
                if (down)
                    config->debug_render_walkmesh ^= true;
                    engine_update_overlay_display(gameEngine);
                break;
            case '[':
                if (down)
                    config->debug_render_collisions ^= true;
                    engine_update_overlay_display(gameEngine);
                break;
            case ']':
                if (down)
                    config->debug_text_triangles ^= true;
                engine_update_overlay_display(gameEngine);
                break;
            case 'u': flags |= INPUT_RESET_CAMERA; break;
        }
    }

    if (down)
        engine_enable_inputs(gameEngine, flags);
    else
        engine_disable_inputs(gameEngine, flags);
}

- (void)keyDown:(NSEvent *)theEvent
{
    if ([theEvent isARepeat])
        return;

    [self updateDiscreteInputsForEvent:theEvent keyDown:YES];
    [self updateAnalogInputsForEvent:theEvent keyDown:YES];
}

- (void)keyUp:(NSEvent *)theEvent
{
    if ([theEvent isARepeat])
        return;

    [self updateDiscreteInputsForEvent:theEvent keyDown:NO];
    [self updateAnalogInputsForEvent:theEvent keyDown:NO];
}

@end
