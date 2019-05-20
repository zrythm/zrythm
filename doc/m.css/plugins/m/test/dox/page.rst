m.dox
#####

.. role:: dox-flat(dox)
    :class: m-flat

-   Function link: :dox:`Utility::Directory::mkpath()`
-   Class link: :dox:`Interconnect::Emitter`
-   Page link: :dox:`building-corrade`
-   :dox:`Custom link title <testsuite>`
-   :dox:`Page link with custom title <corrade-cmake>`
-   :dox:`Link to index page <corrade>`
-   :dox:`Link to class documentation section <TestSuite-Tester-command-line>`
-   :dox:`Link to index page with hash after <corrade#search>`
-   :dox:`Link to page with hash after <corrade-cmake#search>`
-   :dox:`Link to class with query and hash after <Utility::Directory?q=hello#search>`
-   Flat link: :dox-flat:`plugin-management`

These should produce warnings:

-   Link to nonexistent name will be rendered as code: :dox:`nonExistent()`
-   :dox:`Link to nonexistent name with custom title will be just text <nonExistent()>`
-   Link to a section that doesn't have a title will keep the ID (this *may*
    break on tagfile update, watch out): :dox:`corrade-cmake-add-test`
-   Link to index page without title will have the tag file basename:
    :dox:`corrade`
