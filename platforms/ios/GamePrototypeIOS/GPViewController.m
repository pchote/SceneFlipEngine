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

#import "GPViewController.h"
#import "engine.h"

@interface GPViewController ()

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;
@property engine_ptr gameEngine;
@property CGPoint touchStart;

@end

@implementation GPViewController

@synthesize context = _context;
@synthesize effect = _effect;
@synthesize gameEngine = _gameEngine;
@synthesize touchStart = _touchStart;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!self.context)
        NSLog(@"Failed to create ES context");
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;

    [EAGLContext setCurrentContext:self.context];

    CGSize size = [[UIScreen mainScreen] bounds].size;
    GLfloat scale = self.view.contentScaleFactor;

    const char *assetPath = [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"assets"] UTF8String];
    self.gameEngine = engine_create(assetPath, size.height*scale, size.width*scale);
}

- (void)viewDidUnload
{    
    [super viewDidUnload];
    
    [EAGLContext setCurrentContext:self.context];
    engine_destroy(self.gameEngine);
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
	self.context = nil;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc. that aren't in use.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    CGSize size = self.view.bounds.size;
    GLfloat scale = self.view.contentScaleFactor;
    engine_set_viewport(self.gameEngine, size.width*scale, size.height*scale);
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    engine_tick(self.gameEngine, self.timeSinceLastDraw);
    engine_draw(self.gameEngine);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [[event allTouches] anyObject];
    self.touchStart = [touch locationInView:self.view];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchesEnded:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    self.touchStart = CGPointZero;
    engine_set_analog_input(self.gameEngine, ANALOG_INPUT_DIRECTION, (GPpolar){0, 0});
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    GLfloat minRadius = 5;
    GLfloat maxRadius = 50;

    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchPoint = [touch locationInView:self.view];
    GLfloat dx = touchPoint.x - self.touchStart.x;
    GLfloat dy = touchPoint.y - self.touchStart.y;
    GLfloat radius = sqrtf(dx*dx+dy*dy);
    GLfloat angle = atan2f(-dy, dx);

    if (radius > maxRadius)
        radius = maxRadius;
    if (radius < minRadius)
        radius = 0;

    engine_set_analog_input(self.gameEngine, ANALOG_INPUT_DIRECTION, (GPpolar){radius / maxRadius, angle});
}


@end
