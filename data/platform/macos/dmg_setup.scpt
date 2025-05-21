-- SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
-- SPDX-License-Identifier: LicenseRef-ZrythmLicense
-- based on https://www.kitware.com/creating-mac-os-x-packages-with-cmake/
on run argv
  set image_name to item 1 of argv

  tell application "Finder"
  tell disk image_name

    -- wait for the image to finish mounting
    set open_attempts to 0
    repeat while open_attempts < 10
      try
        open
          delay 2
          set open_attempts to 10
        close
      on error errStr number errorNumber
        set open_attempts to open_attempts + 1
        delay 15
      end try
    end repeat
    delay 10

    -- first pass: setup background and icons
    open
      set current view of container window to icon view
      set theViewOptions to the icon view options of container window
      set background picture of theViewOptions to file ".background:background.tif"
      set arrangement of theViewOptions to not arranged
      set icon size of theViewOptions to 128
      delay 10
    close

    -- second pass: position items and hide decorations
    open
      update without registering applications
      tell container window
        set sidebar width to 0
        set statusbar visible to false
        set toolbar visible to false
        set the bounds to { 400, 100, 900, 465 }
        set position of item "Zrythm.app" to { 133, 200 }
        set position of item "Applications" to { 378, 200 }
      end tell
      update without registering applications
      delay 10
    close

    -- one last open and close so you can see everything looks correct
    open
      delay 10
    close

  end tell
  delay 5
end tell
end run
