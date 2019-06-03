#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

static void makeAppMenu (void) {
	id menubar = [[NSMenu new] autorelease];
	id appMenuItem = [[NSMenuItem new] autorelease];
	[menubar addItem:appMenuItem];

	[NSApp setMainMenu:menubar];

	id appMenu = [[NSMenu new] autorelease];
	[appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
	[appMenuItem setSubmenu:appMenu];
}

void rtk_osx_api_init(void) {
	[NSAutoreleasePool new];
	[NSApplication sharedApplication];
	makeAppMenu ();
	//[NSApp setDelegate:[NSApplication sharedApplication]];
	//[NSApp finishLaunching];
}

#ifndef OSX_SHUTDOWN_WAIT
#define OSX_SHUTDOWN_WAIT 100 // run loop callbacks, jack to allow graceful close
#endif

void rtk_osx_api_terminate(void) {
	static int term_from_term = 0;
	if (term_from_term) {
		[[NSApplication sharedApplication] stop:nil];
	}
	if (++term_from_term > OSX_SHUTDOWN_WAIT) {
		[[NSApplication sharedApplication] terminate:nil];
	}
}

void rtk_osx_api_run(void) {
	[NSApp run];
}

void rtk_osx_api_err(const char *msg) {
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:[NSString stringWithUTF8String:msg]];
	[alert runModal];
}

#endif
