/*
  Copyright 2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl_osx.m OSX/Cocoa Pugl Implementation.
*/

#include <stdlib.h>

#import <Cocoa/Cocoa.h>

#include "pugl_internal.h"

// screw apple, their lack of -fvisibility=internal and namespaces
#define CONCAT(A,B) A ## B
#define XCONCAT(A,B) CONCAT(A,B)
#define RobTKPuglWindow XCONCAT(RobTKPuglWindow, UINQHACK)
#define RobTKPuglOpenGLView XCONCAT(RobTKPuglOpenGLView, UINQHACK)

__attribute__ ((visibility ("hidden")))
@interface RobTKPuglWindow : NSWindow
{
@public
	PuglView* puglview;
}

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(unsigned int)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag;
- (void) setPuglview:(PuglView*)view;
- (BOOL) windowShouldClose:(id)sender;
- (void) becomeKeyWindow:(id)sender;
- (BOOL) canBecomeKeyWindow:(id)sender;
@end

@implementation RobTKPuglWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned int)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
	NSWindow* result = [super initWithContentRect:contentRect
					    styleMask:aStyle
					      backing:bufferingType
						defer:NO];

	[result setAcceptsMouseMovedEvents:YES];
	return (RobTKPuglWindow *)result;
}

- (void)setPuglview:(PuglView*)view
{
	puglview = view;
	[self setContentSize:NSMakeSize(view->width, view->height) ];
}

- (BOOL)windowShouldClose:(id)sender
{
	if (puglview->closeFunc)
		puglview->closeFunc(puglview);
	return YES;
}

- (void)becomeKeyWindow:(id)sender
{

}

- (BOOL) canBecomeKeyWindow:(id)sender{
	// forward key-events
	return NO;
}

@end

static void
puglDisplay(PuglView* view)
{
	if (view->displayFunc) {
		view->displayFunc(view);
	}
}

__attribute__ ((visibility ("hidden")))
@interface RobTKPuglOpenGLView : NSOpenGLView
{
@public
	PuglView* puglview;

	NSTrackingArea* trackingArea;
}

- (id) initWithFrame:(NSRect)frame;
- (void) reshape;
- (void) drawRect:(NSRect)rect;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) otherMouseDown:(NSEvent*)event;
- (void) otherMouseUp:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;

@end

@implementation RobTKPuglOpenGLView

- (id) initWithFrame:(NSRect)frame
{
	NSOpenGLPixelFormatAttribute pixelAttribs[16] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 32,
		NSOpenGLPFAMultisample,
		NSOpenGLPFASampleBuffers, 1,
		NSOpenGLPFASamples, 4,
		0
	};
	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
		              initWithAttributes:pixelAttribs];

	if (pixelFormat) {
		self = [super initWithFrame:frame pixelFormat:pixelFormat];
		[pixelFormat release];
	} else {
		self = [super initWithFrame:frame];
	}

	if (self) {
		[[self openGLContext] makeCurrentContext];
		[self reshape];
		[NSOpenGLContext clearCurrentContext];
	}
	return self;
}

- (void) reshape
{
	[[self openGLContext] update];

	NSRect bounds = [self bounds];
	int    width  = bounds.size.width;
	int    height = bounds.size.height;

	if (puglview) {
		/* NOTE: Apparently reshape gets called when the GC gets around to
		   deleting the view (?), so we must have reset puglview to NULL when
		   this comes around.
		*/
		[[self openGLContext] makeCurrentContext];
		if (puglview->reshapeFunc) {
			puglview->reshapeFunc(puglview, width, height);
		} else {
			puglDefaultReshape(puglview, width, height);
		}
		[NSOpenGLContext clearCurrentContext];

		puglview->width  = width;
		puglview->height = height;

		[self removeTrackingArea:trackingArea];
		[trackingArea release];
		trackingArea = nil;
		[self updateTrackingAreas];
	}
}

- (void) drawRect:(NSRect)rect
{
	[[self openGLContext] makeCurrentContext];
	puglDisplay(puglview);
	glFlush();
	glSwapAPPLE();
	[NSOpenGLContext clearCurrentContext];
}

static unsigned
getModifiers(PuglView* view, NSEvent* ev)
{
	const unsigned modifierFlags = [ev modifierFlags];

	view->event_timestamp_ms = fmod([ev timestamp] * 1000.0, UINT32_MAX);

	unsigned mods = 0;
	mods |= (modifierFlags & NSShiftKeyMask)     ? PUGL_MOD_SHIFT : 0;
	mods |= (modifierFlags & NSControlKeyMask)   ? PUGL_MOD_CTRL  : 0;
	mods |= (modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT   : 0;
	mods |= (modifierFlags & NSCommandKeyMask)   ? PUGL_MOD_SUPER : 0;
	return mods;
}

-(void)updateTrackingAreas
{
	if (trackingArea != nil) {
		return;
		[self removeTrackingArea:trackingArea];
		[trackingArea release];
	}

	const int opts = (NSTrackingMouseEnteredAndExited |
	                  NSTrackingMouseMoved |
	                  NSTrackingActiveAlways);
	trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds]
	                                             options:opts
	                                               owner:self
	                                            userInfo:nil];
	[self addTrackingArea:trackingArea];
}

- (void)mouseEntered:(NSEvent*)theEvent
{
	[self updateTrackingAreas];
}

- (void)mouseExited:(NSEvent*)theEvent
{
}

- (void) mouseMoved:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->motionFunc(puglview, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseDragged:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->motionFunc(puglview, loc.x, puglview->height - loc.y);
	}
}

- (void) rightMouseDragged:(NSEvent*)event
{
	if (puglview->motionFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->motionFunc(puglview, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 1, true, loc.x, puglview->height - loc.y);
	}
}

- (void) mouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 1, false, loc.x, puglview->height - loc.y);
	}
	[self updateTrackingAreas];
}

- (void) rightMouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 3, true, loc.x, puglview->height - loc.y);
	}
}

- (void) rightMouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, 3, false, loc.x, puglview->height - loc.y);
	}
}

- (void) otherMouseDown:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, [event buttonNumber], true, loc.x, puglview->height - loc.y);
	}
}

- (void) otherMouseUp:(NSEvent*)event
{
	if (puglview->mouseFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->mouseFunc(puglview, [event buttonNumber], false, loc.x, puglview->height - loc.y);
	}
}

- (void) scrollWheel:(NSEvent*)event
{
	if (puglview->scrollFunc) {
		NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
		puglview->mods = getModifiers(puglview, event);
		puglview->scrollFunc(puglview, loc.x, puglview->height - loc.y, [event deltaX], [event deltaY]);
	}
	[self updateTrackingAreas];
}

- (void) keyDown:(NSEvent*)event
{
	if (puglview->keyboardFunc && !(puglview->ignoreKeyRepeat && [event isARepeat])) {
		NSString* chars = [event characters];
		puglview->mods = getModifiers(puglview, event);
		puglview->keyboardFunc(puglview, true, [chars characterAtIndex:0]);
	}
}

- (void) keyUp:(NSEvent*)event
{
	if (puglview->keyboardFunc) {
		NSString* chars = [event characters];
		puglview->mods = getModifiers(puglview, event);
		puglview->keyboardFunc(puglview, false,  [chars characterAtIndex:0]);
	}
}

- (void) flagsChanged:(NSEvent*)event
{
	if (puglview->specialFunc) {
		const unsigned mods = getModifiers(puglview, event);
		if ((mods & PUGL_MOD_SHIFT) != (puglview->mods & PUGL_MOD_SHIFT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SHIFT, PUGL_KEY_SHIFT);
		} else if ((mods & PUGL_MOD_CTRL) != (puglview->mods & PUGL_MOD_CTRL)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_CTRL, PUGL_KEY_CTRL);
		} else if ((mods & PUGL_MOD_ALT) != (puglview->mods & PUGL_MOD_ALT)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_ALT, PUGL_KEY_ALT);
		} else if ((mods & PUGL_MOD_SUPER) != (puglview->mods & PUGL_MOD_SUPER)) {
			puglview->specialFunc(puglview, mods & PUGL_MOD_SUPER, PUGL_KEY_SUPER);
		}
		puglview->mods = mods;
	}
}

@end

struct PuglInternalsImpl {
	RobTKPuglOpenGLView* glview;
	id                   window;
};

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              min_width,
           int              min_height,
           int              width,
           int              height,
           bool             resizable,
           bool             ontop,
           unsigned long    transientId)
{
	PuglView*      view = (PuglView*)calloc(1, sizeof(PuglView));
	PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));
	if (!view || !impl) {
		return NULL;
	}

	view->impl   = impl;
	view->width  = width;
	view->height = height;
	view->ontop  = ontop;
	view->user_resizable = resizable; // unused

	[NSAutoreleasePool new];
	[NSApplication sharedApplication];

	impl->glview = [RobTKPuglOpenGLView new];
	impl->glview->puglview = view;

	[impl->glview setFrameSize:NSMakeSize(view->width, view->height)];

	if (parent) {
		NSView* pview = (NSView*) parent;
		[pview addSubview:impl->glview];
		[impl->glview setHidden:NO];
		if (!resizable) {
			[impl->glview setAutoresizingMask:NSViewNotSizable];
		}
	} else {
		NSString* titleString = [[NSString alloc]
			initWithBytes:title
			       length:strlen(title)
			     encoding:NSUTF8StringEncoding];
		NSRect frame = NSMakeRect(0, 0, min_width, min_height);
		NSUInteger style = NSClosableWindowMask | NSTitledWindowMask;
		if (resizable) {
			style |= NSResizableWindowMask;
		}
		id window = [[[RobTKPuglWindow alloc]
			initWithContentRect:frame
				  styleMask:style
				    backing:NSBackingStoreBuffered
				      defer:NO
			] retain];
		[window setPuglview:view];
		[window setTitle:titleString];
		puglUpdateGeometryConstraints(view, min_width, min_height, min_width != width);
		if (ontop) {
			[window setLevel: NSStatusWindowLevel];
		}
		impl->window = window;
#if 0
		if (resizable) {
			[impl->glview setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
		}
#endif
		[window setContentView:impl->glview];
		[NSApp activateIgnoringOtherApps:YES];
		[window makeFirstResponder:impl->glview];
		[window makeKeyAndOrderFront:window];
	}
	return view;
}

void
puglDestroy(PuglView* view)
{
	if (!view) {
		return;
	}
	view->impl->glview->puglview = NULL;
	[view->impl->glview removeFromSuperview];
	if (view->impl->window) {
		[view->impl->window close];
	}
	[view->impl->glview release];
	if (view->impl->window) {
		[view->impl->window release];
	}
	free(view->impl);
	free(view);
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	if (view->redisplay) {
		view->redisplay = false;
		[view->impl->glview setNeedsDisplay: YES];
	}

	return PUGL_SUCCESS;
}

static void
puglResize(PuglView* view)
{
	int set_hints; // ignored
	view->resize = false;
	if (!view->resizeFunc) { return; }

	[[view->impl->glview openGLContext] makeCurrentContext];
	view->resizeFunc(view, &view->width, &view->height, &set_hints);
	if (view->impl->window) {
		[view->impl->window setContentSize:NSMakeSize(view->width, view->height) ];
	} else {
		[view->impl->glview setFrameSize:NSMakeSize(view->width, view->height)];
	}
	[view->impl->glview reshape];
	[NSOpenGLContext clearCurrentContext];
}

void
puglPostResize(PuglView* view)
{
	view->resize = true;
	puglResize(view);
}

void
puglShowWindow(PuglView* view)
{
	[view->impl->window setIsVisible:YES];
}

void
puglHideWindow(PuglView* view)
{
	[view->impl->window setIsVisible:NO];
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
	[view->impl->glview setNeedsDisplay: YES];
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return (PuglNativeWindow)view->impl->glview;
}

int
puglOpenFileDialog(PuglView* view, const char *title)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setCanChooseFiles:YES];
	[panel setCanChooseDirectories:NO];
	[panel setAllowsMultipleSelection:NO];

	if (title) {
		NSString* titleString = [[NSString alloc]
			initWithBytes:title
			     	 length:strlen(title)
			   	 encoding:NSUTF8StringEncoding];
		[panel setTitle:titleString];
	}

	[panel beginWithCompletionHandler:^(NSInteger result) {
		bool file_ok = false;
		if (result == NSFileHandlingPanelOKButton) {
			for (NSURL *url in [panel URLs]) {
				if (![url isFileURL]) continue;
				//NSLog(@"%@", url.path);
				const char *fn= [url.path UTF8String];
				file_ok = true;
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, fn);
				}
				break;
			}
		}

		if (!file_ok && view->fileSelectedFunc) {
			view->fileSelectedFunc(view, NULL);
		}
	}];

	return 0;
}

bool
rtk_osx_open_url (const char* url)
{
	NSString* strurl = [[NSString alloc] initWithUTF8String:url];
	NSURL* nsurl = [[NSURL alloc] initWithString:strurl];

	bool ret = [[NSWorkspace sharedWorkspace] openURL:nsurl];

	[strurl release];
	[nsurl release];

	return ret;
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
		[view->impl->window setContentMinSize:NSMakeSize(min_width, min_height)];
		if (aspect) {
			[view->impl->window setContentAspectRatio:NSMakeSize(min_width, min_height)];
		}
		return 0;
}
