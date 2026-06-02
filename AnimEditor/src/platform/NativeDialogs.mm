#import <Cocoa/Cocoa.h>
#include "platform/NativeDialogs.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace anim {

std::string nativeSaveDialog(const std::string& defaultName) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];

        // Set default name
        if (!defaultName.empty()) {
            panel.nameFieldStringValue =
                [NSString stringWithUTF8String:defaultName.c_str()];
        }

        // Filter to .anim files
        panel.allowedFileTypes = @[@"anim"];
        panel.allowsOtherFileTypes = NO;
        panel.title = @"Save Animation";
        panel.message = @"Choose a location to save the animation file.";

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URL;
            return std::string([url.path UTF8String]);
        }
        return {};
    }
}

std::string nativeOpenDialog() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];

        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;

        // Filter to .anim files
        panel.allowedFileTypes = @[@"anim"];
        panel.title = @"Open Animation";
        panel.message = @"Select an animation file to open.";

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = panel.URLs.firstObject;
            return std::string([url.path UTF8String]);
        }
        return {};
    }
}

} // namespace anim

#pragma clang diagnostic pop
