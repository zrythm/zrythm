# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_style STATIC)

set_source_files_properties(
  Style.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(zrythm_style
  URI ZrythmStyle
  VERSION 1.0
  IMPORTS QtQuick.Controls.Basic/6.8 # fallback to Basic
  QML_FILES
    ApplicationWindow.qml
    # Action.qml
    # BusyIndicator.qml
    Button.qml
    ComboBox.qml
    DialogButtonBox.qml
    # Dialog.qml
    # Label.qml
    ItemDelegate.qml
    Menu.qml
    MenuBar.qml
    MenuBarItem.qml
    MenuSeparator.qml
    MenuItem.qml
    # Overlay.qml
    # Page.qml
    # PageIndicator.qml
    # Pane.qml
    Popup.qml
    RoundButton.qml
    # ProgressBar.qml
    # ScrollBar.qml
    # ScrollIndicator.qml
    # ScrollView.qml
    # SwipeView.qml
    # SplitView.qml
    # StackView.qml
    TabBar.qml
    TabButton.qml
    # TextField.qml
    ToolBar.qml
    ToolButton.qml
    ToolSeparator.qml
    ToolTip.qml
    Style.qml

  # compile qml files listed in QML_FILES to C++ using qmltc
  # (fails to compile)
  # ENABLE_TYPE_COMPILER
)

# target_link_libraries(zrythm_style
#   PRIVATE Qt6::QmlPrivate Qt6::QuickPrivate Qt6::Quick
# )