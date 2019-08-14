<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile>
  <compound kind="file">
    <name>doxygen-file.cpp</name>
    <path>/home/mosra/Code/corrade/doc/codingstyle/</path>
    <filename>doxygen-file_8cpp</filename>
  </compound>
  <compound kind="file">
    <name>Array.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>Array_8h</filename>
    <includes id="ArrayView_8h" name="ArrayView.h" local="yes" imported="no">Corrade/Containers/ArrayView.h</includes>
    <includes id="Tags_8h" name="Tags.h" local="yes" imported="no">Corrade/Containers/Tags.h</includes>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <class kind="class">Corrade::Containers::Array</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="typedef">
      <type>ArrayView&lt; T &gt;</type>
      <name>ArrayReference</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aba27f25d31d10b5ffa38960b49e05ed8</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a57f8d4e0a25951f539461539c90d4e25</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a4258433529f847d3e6ed10442f6b9b8b</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ad5f0615634b2553b0cb304aa58da3347</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a9a1ca1efe52f9930f1b31b9feab5a145</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>af8a4e6b17dca34a5eb82b6a8da8c398b</anchor>
      <arglist>(const Array&lt; T &gt; &amp;view)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>ArrayView.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>ArrayView_8h</filename>
    <includes id="Containers_8h" name="Containers.h" local="yes" imported="no">Corrade/Containers/Containers.h</includes>
    <includes id="Assert_8h" name="Assert.h" local="yes" imported="no">Corrade/Utility/Assert.h</includes>
    <class kind="class">Corrade::Containers::ArrayView</class>
    <class kind="class">Corrade::Containers::ArrayView&lt; const void &gt;</class>
    <class kind="class">Corrade::Containers::StaticArrayView</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a32ef8daa386aee742e5f39fc5b1516fd</anchor>
      <arglist>(T *data, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ac8edb9cd0a74f5a0da2ef9c6d03f9ef2</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ac450a49515ff20058552c732542b1b13</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a8e6435208f2cda38505084a930a3e9ec</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a00abca03029b326133e9e7c908508af4</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a9e4a13e421a34a19eec002ed65bda582</anchor>
      <arglist>(StaticArrayView&lt; size_, T &gt;)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aaf76e645cbe62b360a0f13de4dff2837</anchor>
      <arglist>(T(&amp;)[size_])</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a58d7bb8d46f44949b57b226485db4a9b</anchor>
      <arglist>(T *data)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a3cf428b256a5ed8941145e4d15ae8b45</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aee41b58bdb45963c476ce7554dda2290</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aee9c43d476128f05403e638ce539c90d</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Containers.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>Containers_8h</filename>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <class kind="class">Corrade::Containers::Array</class>
    <class kind="class">Corrade::Containers::ArrayView</class>
    <class kind="class">Corrade::Containers::StaticArrayView</class>
    <class kind="class">Corrade::Containers::StaticArray</class>
    <class kind="class">Corrade::Containers::EnumSet</class>
    <class kind="class">Corrade::Containers::LinkedList</class>
    <class kind="class">Corrade::Containers::LinkedListItem</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
  </compound>
  <compound kind="file">
    <name>EnumSet.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>EnumSet_8h</filename>
    <includes id="Containers_8h" name="Containers.h" local="yes" imported="no">Corrade/Containers/Containers.h</includes>
    <includes id="Tags_8h" name="Tags.h" local="yes" imported="no">Corrade/Containers/Tags.h</includes>
    <class kind="class">Corrade::Containers::EnumSet</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_ENUMSET_OPERATORS</name>
      <anchorfile>EnumSet_8h.html</anchorfile>
      <anchor>abea606a97fd1f8321cf005b33239bff0</anchor>
      <arglist>(class)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_ENUMSET_FRIEND_OPERATORS</name>
      <anchorfile>EnumSet_8h.html</anchorfile>
      <anchor>a8d5e08f0c65922f00b53c38ccfcaa99a</anchor>
      <arglist>(class)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>EnumSet.hpp</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>EnumSet_8hpp</filename>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="function">
      <type>Utility::Debug &amp;</type>
      <name>enumSetDebugOutput</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a12c4ea794fa62c56969b1311e2b5c21b</anchor>
      <arglist>(Utility::Debug &amp;debug, EnumSet&lt; T, fullValue &gt; value, const char *empty, std::initializer_list&lt; T &gt; enums)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>LinkedList.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>LinkedList_8h</filename>
    <includes id="Containers_8h" name="Containers.h" local="yes" imported="no">Corrade/Containers/Containers.h</includes>
    <includes id="Assert_8h" name="Assert.h" local="yes" imported="no">Corrade/Utility/Assert.h</includes>
    <class kind="class">Corrade::Containers::LinkedList</class>
    <class kind="class">Corrade::Containers::LinkedListItem</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
  </compound>
  <compound kind="file">
    <name>StaticArray.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>StaticArray_8h</filename>
    <includes id="ArrayView_8h" name="ArrayView.h" local="yes" imported="no">Corrade/Containers/ArrayView.h</includes>
    <includes id="Tags_8h" name="Tags.h" local="yes" imported="no">Corrade/Containers/Tags.h</includes>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <class kind="class">Corrade::Containers::StaticArray</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aa8111199cdb6cc30c470e3cc9c54a30e</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a5b40a450ade4927381bf97a165e3621e</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a4f6c86de70a1ab496a1d798648c22f80</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, const T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ab15dc352f75f85e562eaa85ae058ac9f</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a5ebf447d7f388e81fe6d34910c80cfbe</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), const U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aae0b8c37cdca03924fac3a3ff457bf9b</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a0cad6e09b65666040bb2460df883e4a3</anchor>
      <arglist>(const StaticArray&lt; size_, T &gt; &amp;)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Tags.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Containers/</path>
    <filename>Tags_8h</filename>
    <class kind="struct">Corrade::Containers::DefaultInitT</class>
    <class kind="struct">Corrade::Containers::ValueInitT</class>
    <class kind="struct">Corrade::Containers::NoInitT</class>
    <class kind="struct">Corrade::Containers::DirectInitT</class>
    <class kind="struct">Corrade::Containers::InPlaceInitT</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Containers</namespace>
    <member kind="variable">
      <type>constexpr DefaultInitT</type>
      <name>DefaultInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a10f8c95aaa0d51bb77976fe3e4924a5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr ValueInitT</type>
      <name>ValueInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a26d49cbb15d20d7dc72878f522e24444</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr NoInitT</type>
      <name>NoInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a7094104336b357363773b3f29891d048</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr DirectInitT</type>
      <name>DirectInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a69da0a827c69b45cec539ec0c201a119</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr InPlaceInitT</type>
      <name>InPlaceInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ab86b6e81b5157279ebfbf2b716fb2d6d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Corrade.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/</path>
    <filename>Corrade_8h</filename>
    <namespace>Corrade</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_GCC47_COMPATIBILITY</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a5e970faf1046ab09ae0cc2ee5ec3f029</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_MSVC2017_COMPATIBILITY</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a553f541a45d293e05e7aebb726ed49aa</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_MSVC2015_COMPATIBILITY</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>abf7db3f63bdd0deb0fb40a9555e8c56a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_BUILD_DEPRECATED</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a3768587aab2fe63f56887ce5367594d5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_BUILD_STATIC</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a18e68fa3008fa4f6f734cc69cb94c133</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_UNIX</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a89eaf7bef5858f7e783a4e57aec81c6f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_APPLE</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>ae2cc32d00e885ea5101b9397ca3bb384</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_IOS</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a28ef41f421f7abcd207c8da78fc3c1da</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_IOS_SIMULATOR</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>abd967ec380b0645c82d7d0cf17106313</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_WINDOWS</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a19063435bd2d1b9a6a2567c8c316cb56</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_WINDOWS_RT</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>aa0551725e55f4584267108f34d07174a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_EMSCRIPTEN</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>aafeb56971752236fd87733937f9392f6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_ANDROID</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a4d1ec6acb651e40f04ad8aef103f51a2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_X86</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>ae205d17d7e08d468eb9fd141467c3f14</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TARGET_ARM</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>a197c45a30d0cc37b5ffecb9943cdfb9a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_UTILITY_USE_ANSI_COLORS</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>ac7cb3a2eb43e3bc3e9e925899229a768</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TESTSUITE_TARGET_XCTEST</name>
      <anchorfile>Corrade_8h.html</anchorfile>
      <anchor>ad07f99b330b2e84932d57b2cb7ed393d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>AbstractFilesystem.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Filesystem/</path>
    <filename>AbstractFilesystem_8h</filename>
    <includes id="AbstractPlugin_8h" name="AbstractPlugin.h" local="yes" imported="no">Corrade/PluginManager/AbstractPlugin.h</includes>
    <class kind="class">Corrade::Filesystem::AbstractFilesystem</class>
    <namespace>Corrade</namespace>
  </compound>
  <compound kind="file">
    <name>Connection.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Interconnect/</path>
    <filename>Connection_8h</filename>
    <includes id="Interconnect_8h" name="Interconnect.h" local="yes" imported="no">Corrade/Interconnect/Interconnect.h</includes>
    <class kind="class">Corrade::Interconnect::Connection</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Interconnect</namespace>
  </compound>
  <compound kind="file">
    <name>Emitter.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Interconnect/</path>
    <filename>Emitter_8h</filename>
    <includes id="Connection_8h" name="Connection.h" local="yes" imported="no">Corrade/Interconnect/Connection.h</includes>
    <includes id="Assert_8h" name="Assert.h" local="yes" imported="no">Corrade/Utility/Assert.h</includes>
    <class kind="class">Corrade::Interconnect::Emitter</class>
    <class kind="class">Corrade::Interconnect::Emitter::Signal</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Interconnect</namespace>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a9ede79fbd60785ea83ffed32762d02ab</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), void(*slot)(Args...))</arglist>
    </member>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a7476864056b3ca1a98430d5caef0e4b4</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), Lambda slot)</arglist>
    </member>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a0d313f017168b64b817565ecb7100299</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), ReceiverObject &amp;receiver, void(Receiver::*slot)(Args...))</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Interconnect.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Interconnect/</path>
    <filename>Interconnect_8h</filename>
    <class kind="class">Corrade::Interconnect::StateMachine</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Interconnect</namespace>
  </compound>
  <compound kind="file">
    <name>Receiver.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Interconnect/</path>
    <filename>Receiver_8h</filename>
    <includes id="Interconnect_8h" name="Interconnect.h" local="yes" imported="no">Corrade/Interconnect/Interconnect.h</includes>
    <class kind="class">Corrade::Interconnect::Receiver</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Interconnect</namespace>
  </compound>
  <compound kind="file">
    <name>AbstractManager.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>AbstractManager_8h</filename>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <includes id="PluginMetadata_8h" name="PluginMetadata.h" local="yes" imported="no">Corrade/PluginManager/PluginMetadata.h</includes>
    <includes id="PluginManager_8h" name="PluginManager.h" local="yes" imported="no">Corrade/PluginManager/PluginManager.h</includes>
    <includes id="Configuration_8h" name="Configuration.h" local="yes" imported="no">Corrade/Utility/Configuration.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <includes id="Resource_8h" name="Resource.h" local="yes" imported="no">Corrade/Utility/Resource.h</includes>
    <class kind="class">Corrade::PluginManager::AbstractManager</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_PLUGIN_IMPORT</name>
      <anchorfile>AbstractManager_8h.html</anchorfile>
      <anchor>a4ced3bca898533c1f46abcd6ec64342b</anchor>
      <arglist>(name)</arglist>
    </member>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; LoadState &gt;</type>
      <name>LoadStates</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>a61b14635bc0c147c65e8be80260ca891</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>LoadState</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>aa240e935222a178e1acb5c0853f15547</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a38c300f4fc9ce8a77aad4a30de05cad8">NotFound</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3485912f892918a4ed6b357abad8e5a4">WrongPluginVersion</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ae90c7458dc20c6326d3a34071de85e3f">WrongInterfaceVersion</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ad6ab066e75221cc9a69bfede5f75347c">WrongMetadataFile</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3c062ee3a4f39445ef4ba44ada5bde3c">UnresolvedDependency</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3729d9667c9ed82ae96b6174b288a3a5">LoadFailed</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a84a8921b25f505d0d2077aeb5db4bc16">Static</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a7381d487d18845b379422325c0a768d6">Loaded</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a5111e24c1ecc6266ce0de4b4dc42033b">NotLoaded</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ac7de13f47f40f470d0795d224221a577">UnloadFailed</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ab651efdb98a5d6bd2b3935d0c3f4a5e2">Required</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a019d1ca7d50cc54b995f60d456435e87">Used</enumvalue>
    </member>
    <member kind="function">
      <type>Utility::Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>a73b06a0cd1333798b796a2061dc6bedd</anchor>
      <arglist>(Utility::Debug &amp;debug, PluginManager::LoadState value)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>AbstractManagingPlugin.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>AbstractManagingPlugin_8h</filename>
    <includes id="AbstractPlugin_8h" name="AbstractPlugin.h" local="yes" imported="no">Corrade/PluginManager/AbstractPlugin.h</includes>
    <includes id="Manager_8h" name="Manager.h" local="yes" imported="no">Corrade/PluginManager/Manager.h</includes>
    <class kind="class">Corrade::PluginManager::AbstractManagingPlugin</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
  </compound>
  <compound kind="file">
    <name>AbstractPlugin.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>AbstractPlugin_8h</filename>
    <includes id="AbstractManager_8h" name="AbstractManager.h" local="yes" imported="no">Corrade/PluginManager/AbstractManager.h</includes>
    <class kind="class">Corrade::PluginManager::AbstractPlugin</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_PLUGIN_VERSION</name>
      <anchorfile>AbstractPlugin_8h.html</anchorfile>
      <anchor>a32a64a27920c39102eec2496320a0b8c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_PLUGIN_INTERFACE</name>
      <anchorfile>AbstractPlugin_8h.html</anchorfile>
      <anchor>a361c0144a9a2664c32259912fdc4cdc2</anchor>
      <arglist>(name)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_PLUGIN_REGISTER</name>
      <anchorfile>AbstractPlugin_8h.html</anchorfile>
      <anchor>aa39d441432b9f4e399c152c85a6dd417</anchor>
      <arglist>(name, className, interface)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Manager.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>Manager_8h</filename>
    <includes id="AbstractManager_8h" name="AbstractManager.h" local="yes" imported="no">Corrade/PluginManager/AbstractManager.h</includes>
    <class kind="class">Corrade::PluginManager::Manager</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
  </compound>
  <compound kind="file">
    <name>PluginManager.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>PluginManager_8h</filename>
    <class kind="class">Corrade::PluginManager::AbstractManagingPlugin</class>
    <class kind="class">Corrade::PluginManager::Manager</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
  </compound>
  <compound kind="file">
    <name>PluginMetadata.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/PluginManager/</path>
    <filename>PluginMetadata_8h</filename>
    <includes id="PluginManager_8h" name="PluginManager.h" local="yes" imported="no">Corrade/PluginManager/PluginManager.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::PluginManager::PluginMetadata</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::PluginManager</namespace>
  </compound>
  <compound kind="file">
    <name>Comparator.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/</path>
    <filename>Comparator_8h</filename>
    <includes id="Assert_8h" name="Assert.h" local="yes" imported="no">Corrade/Utility/Assert.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <class kind="class">Corrade::TestSuite::Comparator</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
  </compound>
  <compound kind="file">
    <name>Container.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>Container_8h</filename>
    <includes id="Comparator_8h" name="Comparator.h" local="yes" imported="no">Corrade/TestSuite/Comparator.h</includes>
    <class kind="class">Corrade::TestSuite::Compare::Container</class>
    <class kind="class">Corrade::TestSuite::Compare::SortedContainer</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::TestSuite::Compare</namespace>
  </compound>
  <compound kind="file">
    <name>File.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>File_8h</filename>
    <includes id="TestSuite_8h" name="TestSuite.h" local="yes" imported="no">Corrade/TestSuite/TestSuite.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::TestSuite::Compare::File</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::TestSuite::Compare</namespace>
  </compound>
  <compound kind="file">
    <name>FileToString.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>FileToString_8h</filename>
    <includes id="TestSuite_8h" name="TestSuite.h" local="yes" imported="no">Corrade/TestSuite/TestSuite.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::TestSuite::Compare::FileToString</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::TestSuite::Compare</namespace>
  </compound>
  <compound kind="file">
    <name>FloatingPoint.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>FloatingPoint_8h</filename>
    <includes id="TestSuite_8h" name="TestSuite.h" local="yes" imported="no">Corrade/TestSuite/TestSuite.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::TestSuite::Comparator&lt; float &gt;</class>
    <class kind="class">Corrade::TestSuite::Comparator&lt; double &gt;</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
  </compound>
  <compound kind="file">
    <name>Numeric.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>Numeric_8h</filename>
    <includes id="TestSuite_8h" name="TestSuite.h" local="yes" imported="no">Corrade/TestSuite/TestSuite.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <class kind="class">Corrade::TestSuite::Compare::Less</class>
    <class kind="class">Corrade::TestSuite::Compare::LessOrEqual</class>
    <class kind="class">Corrade::TestSuite::Compare::GreaterOrEqual</class>
    <class kind="class">Corrade::TestSuite::Compare::Greater</class>
    <class kind="class">Corrade::TestSuite::Compare::Around</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::TestSuite::Compare</namespace>
    <member kind="function">
      <type>Around&lt; T &gt;</type>
      <name>around</name>
      <anchorfile>namespaceCorrade_1_1TestSuite_1_1Compare.html</anchorfile>
      <anchor>a76165bdc14d6ddd8ba20356ffd11e2e5</anchor>
      <arglist>(T epsilon)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>StringToFile.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/Compare/</path>
    <filename>StringToFile_8h</filename>
    <includes id="TestSuite_8h" name="TestSuite.h" local="yes" imported="no">Corrade/TestSuite/TestSuite.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::TestSuite::Compare::StringToFile</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::TestSuite::Compare</namespace>
  </compound>
  <compound kind="file">
    <name>Tester.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/</path>
    <filename>Tester_8h</filename>
    <includes id="Comparator_8h" name="Comparator.h" local="yes" imported="no">Corrade/TestSuite/Comparator.h</includes>
    <includes id="FloatingPoint_8h" name="FloatingPoint.h" local="yes" imported="no">Corrade/TestSuite/Compare/FloatingPoint.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <class kind="class">Corrade::TestSuite::Tester</class>
    <class kind="class">Corrade::TestSuite::Tester::TesterConfiguration</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_TEST_MAIN</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>a3c85abc9a7086c802f27679aaa18857d</anchor>
      <arglist>(Class)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_VERIFY</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>abece56caafaac24db5acc8d2cbcdacf8</anchor>
      <arglist>(expression)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_COMPARE</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>ac040e8cda2279e803f5cf7ae26fef454</anchor>
      <arglist>(actual, expected)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_COMPARE_AS</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>a14d3e517a402fb78ccd2535cd02a3ffe</anchor>
      <arglist>(actual, expected, Type...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_COMPARE_WITH</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>a100e93eb343f155d19f9af61176c3fd9</anchor>
      <arglist>(actual, expected, comparatorInstance)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_EXPECT_FAIL</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>abe5c7d7ccc6a5dbb67a4be57ae5ad87f</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_EXPECT_FAIL_IF</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>a541129fcf90cb5371553ad0d056ac2cf</anchor>
      <arglist>(condition, message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_SKIP</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>a133aa626c0c87b8c1a600a3a74594c84</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_BENCHMARK</name>
      <anchorfile>Tester_8h.html</anchorfile>
      <anchor>accfe2c844655169d9d546734b3d9bc12</anchor>
      <arglist>(batchSize)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>TestSuite.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/TestSuite/</path>
    <filename>TestSuite_8h</filename>
    <class kind="class">Corrade::TestSuite::Comparator</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::TestSuite</namespace>
  </compound>
  <compound kind="file">
    <name>AbstractHash.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>AbstractHash_8h</filename>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <class kind="class">Corrade::Utility::HashDigest</class>
    <class kind="class">Corrade::Utility::AbstractHash</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>AndroidStreamBuffer.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>AndroidStreamBuffer_8h</filename>
    <class kind="class">Corrade::Utility::AndroidLogStreamBuffer</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>Arguments.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Arguments_8h</filename>
    <includes id="ConfigurationValue_8h" name="ConfigurationValue.h" local="yes" imported="no">Corrade/Utility/ConfigurationValue.h</includes>
    <class kind="class">Corrade::Utility::Arguments</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>Assert.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Assert_8h</filename>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_ASSERT</name>
      <anchorfile>Assert_8h.html</anchorfile>
      <anchor>a83f7361970111951c88f1564a4f148e8</anchor>
      <arglist>(condition, message, returnValue)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_INTERNAL_ASSERT</name>
      <anchorfile>Assert_8h.html</anchorfile>
      <anchor>a1114c0fdae83004b9a0f8d2ed4afae96</anchor>
      <arglist>(condition)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_INTERNAL_ASSERT_OUTPUT</name>
      <anchorfile>Assert_8h.html</anchorfile>
      <anchor>a8154b41da944a95b2aab65686a1bc249</anchor>
      <arglist>(call)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_ASSERT_UNREACHABLE</name>
      <anchorfile>Assert_8h.html</anchorfile>
      <anchor>a4cb150db08b529a855e172e3c06e0e71</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Configuration.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Configuration_8h</filename>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <includes id="ConfigurationGroup_8h" name="ConfigurationGroup.h" local="yes" imported="no">Corrade/Utility/ConfigurationGroup.h</includes>
    <class kind="class">Corrade::Utility::Configuration</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>ConfigurationGroup.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>ConfigurationGroup_8h</filename>
    <includes id="ConfigurationValue_8h" name="ConfigurationValue.h" local="yes" imported="no">Corrade/Utility/ConfigurationValue.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::Utility::ConfigurationGroup</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>ConfigurationValue.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>ConfigurationValue_8h</filename>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <class kind="struct">Corrade::Utility::ConfigurationValue</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; short &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned short &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; int &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned int &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned long long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; float &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; double &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long double &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; std::string &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; bool &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; char32_t &gt;</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; ConfigurationValueFlag &gt;</type>
      <name>ConfigurationValueFlags</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a4d1891e33318b3189e012812127acd8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>ConfigurationValueFlag</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>acac747502f9bde1bf73cbfbe661c780d</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da594be08882c8e9d5efb9eeb62f303744">Oct</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da92640bd72988395b326c888614f8937a">Hex</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da21234a0e100d74037a4da2e53f3200d7">Scientific</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da621e7b8ece62fecc55e883252ff2fbe7">Uppercase</enumvalue>
    </member>
  </compound>
  <compound kind="file">
    <name>Debug.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Debug_8h</filename>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <includes id="TypeTraits_8h" name="TypeTraits.h" local="yes" imported="no">Corrade/Utility/TypeTraits.h</includes>
    <includes id="Utility_8h" name="Utility.h" local="yes" imported="no">Corrade/Utility/Utility.h</includes>
    <class kind="class">Corrade::Utility::Debug</class>
    <class kind="class">Corrade::Utility::Warning</class>
    <class kind="class">Corrade::Utility::Error</class>
    <class kind="class">Corrade::Utility::Fatal</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a0cd339017bd5cf30f3374f370cd8f19c</anchor>
      <arglist>(Debug &amp;debug, Debug::Color value)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Directory.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Directory_8h</filename>
    <includes id="Containers_8h" name="Containers.h" local="yes" imported="no">Corrade/Containers/Containers.h</includes>
    <includes id="EnumSet_8h" name="EnumSet.h" local="yes" imported="no">Corrade/Containers/EnumSet.h</includes>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <namespace>Corrade::Utility::Directory</namespace>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; Flag &gt;</type>
      <name>Flags</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a9df63241a8c57cdd64316a82bc9713b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>Flag</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a60fc75369f2d2d700d1cc758261cef35</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35ad6186b962ccb090e812467363c008e0a">SkipDotAndDotDot</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35ab5fc8c3926425785013c492ab8ecc22f">SkipFiles</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a9011abdfaf22b80f4ae98a3f59fed46b">SkipDirectories</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a2e58e2c745e14cc46a112c48075b863e">SkipSpecial</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a6e1549d1367460b4ab06f5575f2d427c">SortAscending</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a12995c419287e0ef1f6f2d6ce1dfc12a">SortDescending</enumvalue>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>fromNativeSeparators</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a454691f5180e746e5f37d50e5aab9f13</anchor>
      <arglist>(std::string path)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>toNativeSeparators</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ae912c293390240c7c55cf36a3dd64fc3</anchor>
      <arglist>(std::string path)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>path</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a222baead7128ec0cadc433e3a8c0059f</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>filename</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a07df998aa551057785f2db6c51e78e63</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>join</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>aa1c57ae6d2c15c7507ad1ae70810d4de</anchor>
      <arglist>(const std::string &amp;path, const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>list</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a27708f99ad155cdf20dbfb7926478582</anchor>
      <arglist>(const std::string &amp;path, Flags flags=Flags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>mkpath</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ad80859f373fbf1ed39b11eb27649c34b</anchor>
      <arglist>(const std::string &amp;path)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>rm</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ac78c147e3378045ca84362dc1617a5d2</anchor>
      <arglist>(const std::string &amp;path)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>move</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>aa3505c07378a35988f43dca3496f9631</anchor>
      <arglist>(const std::string &amp;oldPath, const std::string &amp;newPath)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isSandboxed</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a3cc7a87483e3b192253cbe72d1cb1f98</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>executableLocation</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a4e3dbc86c7b8d0c3c39d9802a9947b35</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>home</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a8dcea7e885d165d77f8ef68814788bd9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>configurationDir</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a73ddfdc8249b1bc779e8bb58cd9c319a</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>tmp</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a7b3ca5990decccd118261fe86824af70</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>fileExists</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a1a83e17a2230dd6c04d33dac2a6e745f</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; char &gt;</type>
      <name>read</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a4a9da22f34b0eb0f409b8183032b46db</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>readString</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>afdc9b2319f9470117c4fb6b2ecf63187</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>write</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>af3e40dd0b19be0b79e8d2bad631f9009</anchor>
      <arglist>(const std::string &amp;filename, Containers::ArrayView&lt; const void &gt; data)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>writeString</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a2e31a8897f1f4d78ec7179d584ab0100</anchor>
      <arglist>(const std::string &amp;filename, const std::string &amp;data)</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; char, MapDeleter &gt;</type>
      <name>map</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a769141becd7eed77b15c0d0d1a79cc04</anchor>
      <arglist>(const std::string &amp;filename, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; const char, MapDeleter &gt;</type>
      <name>mapRead</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a84c45342a8cec2111853f51873d82c9d</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Endianness.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Endianness_8h</filename>
    <includes id="utilities_8h" name="utilities.h" local="yes" imported="no">Corrade/Utility/utilities.h</includes>
    <class kind="class">Corrade::Utility::Endianness</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>Macros.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Macros_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_DEPRECATED</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>a8bc844974522ec40054a604cadbdb2b2</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_DEPRECATED_ALIAS</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>a023a57e9d0c61a1b6f840b3e0569f734</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_DEPRECATED_ENUM</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>aa58b39f18ce825eaad362ea65e91eda1</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_DEPRECATED_FILE</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>a4bc9f2298919bde9bacc16f319707fe3</anchor>
      <arglist>(message)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_UNUSED</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>aaf99b187cce2ff1eadddf1507ace2d69</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_ALIGNAS</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>acb148d614b069112fb97d4f022ab6c9b</anchor>
      <arglist>(alignment)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_NORETURN</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>aeac429c30ec933b14293d5431a5c1a01</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_AUTOMATIC_INITIALIZER</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>a8a4cea6b4e9cc0f231fec08fc867f2d9</anchor>
      <arglist>(function)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_AUTOMATIC_FINALIZER</name>
      <anchorfile>Macros_8h.html</anchorfile>
      <anchor>a89fc5f07629645f4e45c130859ac0909</anchor>
      <arglist>(function)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>MurmurHash2.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>MurmurHash2_8h</filename>
    <includes id="AbstractHash_8h" name="AbstractHash.h" local="yes" imported="no">Corrade/Utility/AbstractHash.h</includes>
    <class kind="class">Corrade::Utility::MurmurHash2</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>rc.cpp</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>rc_8cpp</filename>
    <includes id="Arguments_8h" name="Arguments.h" local="yes" imported="no">Corrade/Utility/Arguments.h</includes>
    <includes id="Debug_8h" name="Debug.h" local="yes" imported="no">Corrade/Utility/Debug.h</includes>
    <includes id="Directory_8h" name="Directory.h" local="yes" imported="no">Corrade/Utility/Directory.h</includes>
    <includes id="Resource_8h" name="Resource.h" local="yes" imported="no">Corrade/Utility/Resource.h</includes>
  </compound>
  <compound kind="file">
    <name>Resource.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Resource_8h</filename>
    <includes id="ArrayView_8h" name="ArrayView.h" local="yes" imported="no">Corrade/Containers/ArrayView.h</includes>
    <class kind="class">Corrade::Utility::Resource</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_RESOURCE_INITIALIZE</name>
      <anchorfile>Resource_8h.html</anchorfile>
      <anchor>a876986c779d3629c74c76e35607ea9e2</anchor>
      <arglist>(name)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_RESOURCE_FINALIZE</name>
      <anchorfile>Resource_8h.html</anchorfile>
      <anchor>a6ded4adbc1776241a0aed878fa5207a9</anchor>
      <arglist>(name)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Sha1.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Sha1_8h</filename>
    <includes id="AbstractHash_8h" name="AbstractHash.h" local="yes" imported="no">Corrade/Utility/AbstractHash.h</includes>
    <class kind="class">Corrade::Utility::Sha1</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>String.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>String_8h</filename>
    <includes id="ArrayView_8h" name="ArrayView.h" local="yes" imported="no">Corrade/Containers/ArrayView.h</includes>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <namespace>Corrade::Utility::String</namespace>
    <member kind="function">
      <type>std::string</type>
      <name>fromArray</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a3e5a23b4c3afe46b19c0ea93a098e634</anchor>
      <arglist>(const char *string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>fromArray</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>acb24339bd65dda380b696d363f4cc799</anchor>
      <arglist>(const char *string, std::size_t length)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>afc39c7e3c0d3da5bb4cfdf4bada808ab</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a9b120b72f24ac6c1266d60b112a131f6</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a5dcd2f1e07dfeb6b3f523b8fd7fcc0db</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>aa7519775e721a492a57c3752b6e0a5ae</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a787500cae52742d206c30a00d315bd88</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a13384c2eb91136a2282cabcf6d40e7e7</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a010626127bd50cc902768b3fa50e8e8c</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a3e6ed45cb59343ff1c968cc7c5f74864</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ab1422c3f09858ed4959d7ddd022c913a</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>split</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac33538e87b593ea3025b97cb9c4f9c7d</anchor>
      <arglist>(const std::string &amp;string, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a223a1c3ccc10a0a1c0d3cea7e1bb8770</anchor>
      <arglist>(const std::string &amp;string, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac0ff6b03a281fecebe8540c3aa86cd47</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;delimiters)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a1360ec5f0e3aa9a34af046480356292f</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;delimiters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ab0ad1369cf991d4a24ad9eb546da5994</anchor>
      <arglist>(const std::string &amp;string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>join</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a9af3018407a9cac247c09da9ac8e577d</anchor>
      <arglist>(const std::vector&lt; std::string &gt; &amp;strings, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>joinWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac2adb8eafce8ddb6f7c163bd112e5c57</anchor>
      <arglist>(const std::vector&lt; std::string &gt; &amp;strings, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>lowercase</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a09fde2ac2c5c7a28e0c694d22ca89ee6</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>uppercase</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a4593d63bc7b1f340584b21d6a169afe9</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>beginsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>add091001179ac1455795365a3b667073</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;prefix)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>beginsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>aca63ac7667c9b71ce5e3240daf32316e</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;prefix)[size])</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>endsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a810b5cb07e3a5e5063145ef47567ab7f</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;suffix)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>endsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ace94f739418cde7292a1e4ffdb75598e</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;suffix)[size])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>System.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>System_8h</filename>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <namespace>Corrade::Utility::System</namespace>
    <member kind="function">
      <type>void</type>
      <name>sleep</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1System.html</anchorfile>
      <anchor>a7f93179c285d2667b97d9b38476d4cda</anchor>
      <arglist>(std::size_t ms)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>TypeTraits.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>TypeTraits_8h</filename>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_HAS_TYPE</name>
      <anchorfile>TypeTraits_8h.html</anchorfile>
      <anchor>a19ce628bb598766db0fffb24c19fc9de</anchor>
      <arglist>(className, typeExpression)</arglist>
    </member>
    <member kind="typedef">
      <type>std::integral_constant&lt; bool,(Implementation::HasMemberBegin&lt; T &gt;::value||Implementation::HasBegin&lt; T &gt;::value||Implementation::HasStdBegin&lt; T &gt;::value)&amp;&amp;(Implementation::HasMemberEnd&lt; T &gt;::value||Implementation::HasEnd&lt; T &gt;::value||Implementation::HasStdEnd&lt; T &gt;::value)&gt;</type>
      <name>IsIterable</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a43561aa129a33d6c7aa6cb828dacdf46</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Unicode.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Unicode_8h</filename>
    <includes id="ArrayView_8h" name="ArrayView.h" local="yes" imported="no">Corrade/Containers/ArrayView.h</includes>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <namespace>Corrade::Utility::Unicode</namespace>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a23fbffa5a6184ab6c29030e152182c29</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text, std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a3cd253f620e843e9ecac1d427b7c070f</anchor>
      <arglist>(const std::string &amp;text, const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a634c36aab0be25e8417905c1a1b492c4</anchor>
      <arglist>(const char(&amp;text)[size], const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ab5eed1f9f89a2e19eaef0c0f64fb9900</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text, std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ad231509de72398162b11d48f9ea75f85</anchor>
      <arglist>(const std::string &amp;text, const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ab7b8f89831f52d67d311c5886b847ce6</anchor>
      <arglist>(const char(&amp;text)[size], const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::u32string</type>
      <name>utf32</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a738435f73e123822a70f692d9d9cfec8</anchor>
      <arglist>(const std::string &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>utf8</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a0b1547a3b650117161826a75943e5946</anchor>
      <arglist>(char32_t character, Containers::StaticArrayView&lt; 4, char &gt; result)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>af92cf003d4c9bf36b0a53777ab5a0445</anchor>
      <arglist>(const std::string &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>aa4f142c6268dbd30d3b816e36e9ab37b</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a025f6762f095b8180ce0569573b82380</anchor>
      <arglist>(const char *text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ad81ae601a86426189543698218fb276d</anchor>
      <arglist>(const std::wstring &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a68b980fd21405108c7eb80d1b07408e6</anchor>
      <arglist>(Containers::ArrayView&lt; const wchar_t &gt; text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a7dfc7f84250f3a9a16f4a8fb4bc7a97b</anchor>
      <arglist>(const wchar_t *text)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>utilities.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>utilities_8h</filename>
    <includes id="Macros_8h" name="Macros.h" local="yes" imported="no">Corrade/Utility/Macros.h</includes>
    <includes id="System_8h" name="System.h" local="yes" imported="no">Corrade/Utility/System.h</includes>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
    <member kind="function">
      <type>void</type>
      <name>sleep</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a0e8ee1cd790d25f505ee1405ec15e238</anchor>
      <arglist>(std::size_t ms)</arglist>
    </member>
    <member kind="function">
      <type>To</type>
      <name>bitCast</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a3831cece822dfde455336664676f6867</anchor>
      <arglist>(const From &amp;from)</arglist>
    </member>
    <member kind="function">
      <type>To</type>
      <name>bitCast</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a3831cece822dfde455336664676f6867</anchor>
      <arglist>(const From &amp;from)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>Utility.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>Utility_8h</filename>
    <includes id="Containers_8h" name="Containers.h" local="yes" imported="no">Corrade/Containers/Containers.h</includes>
    <class kind="class">Corrade::Utility::HashDigest</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue</class>
    <namespace>Corrade</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="file">
    <name>VisibilityMacros.h</name>
    <path>/home/mosra/Code/corrade/src/Corrade/Utility/</path>
    <filename>VisibilityMacros_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_VISIBILITY_EXPORT</name>
      <anchorfile>VisibilityMacros_8h.html</anchorfile>
      <anchor>a94f5b366975d677886021cd41bfaacfb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_VISIBILITY_IMPORT</name>
      <anchorfile>VisibilityMacros_8h.html</anchorfile>
      <anchor>a62c8c1098f4007c1769d03181a78b33c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_VISIBILITY_STATIC</name>
      <anchorfile>VisibilityMacros_8h.html</anchorfile>
      <anchor>ac12ca04badf8b2b6bc283e5f0da3802e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CORRADE_VISIBILITY_LOCAL</name>
      <anchorfile>VisibilityMacros_8h.html</anchorfile>
      <anchor>a802c9a3721942bf48b11ca6314e14a4a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>AbstractHash&lt; 20 &gt;</name>
    <filename>classCorrade_1_1Utility_1_1AbstractHash.html</filename>
    <member kind="typedef">
      <type>HashDigest&lt; digestSize &gt;</type>
      <name>Digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a44db12441a55b94fdead99c880fd0a4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DigestSize</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a162fa31a26bf6c06ed8d23c417e2020ca1845bcbe98f41a624c92ed7e797a14f6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>AbstractHash&lt; sizeof(std::size_t)&gt;</name>
    <filename>classCorrade_1_1Utility_1_1AbstractHash.html</filename>
    <member kind="typedef">
      <type>HashDigest&lt; digestSize &gt;</type>
      <name>Digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a44db12441a55b94fdead99c880fd0a4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DigestSize</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a162fa31a26bf6c06ed8d23c417e2020ca1845bcbe98f41a624c92ed7e797a14f6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Array&lt; char &gt;</name>
    <filename>classCorrade_1_1Containers_1_1Array.html</filename>
    <member kind="typedef">
      <type>char</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab88bec3b842462e9eea5157a4049a094</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void(*)(char *, std::size_t)</type>
      <name>Deleter</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a6397e3aabed2ca448871a747e19a7ee7</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae5ab628ed1d689df54ee1e14c9f33e00</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab4c2451e1504971dee5076ad43300be8</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a4fcbcb24d5a462f13e7dc8e138664e31</anchor>
      <arglist>(DefaultInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ada8220cec2457b9e1730b71d24e3e60e</anchor>
      <arglist>(ValueInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ad6a7e145daae045d737003b626cc958d</anchor>
      <arglist>(NoInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ad0e6ed64bc694f8e4b316848314c5345</anchor>
      <arglist>(DirectInitT, std::size_t size, Args &amp;&amp;...args)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa48105e1a6a95c402c274d32d8d213c1</anchor>
      <arglist>(InPlaceInitT, std::initializer_list&lt; char &gt; list)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a913499c6a05cd36964ad56d0eaa1f002</anchor>
      <arglist>(std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab527926a8480885ca15d7fd53c4e0cef</anchor>
      <arglist>(char *data, std::size_t size, void(*)(char *, std::size_t)deleter=Implementation::DefaultDeleter&lt; void(*)(char *, std::size_t) &gt;{}())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a96d5213dcca0193ecd6c8d67217efde2</anchor>
      <arglist>(const Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ade7103081fcb82b77f40b3a6a0c9bd1f</anchor>
      <arglist>(Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type>Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae4fd95d4d6fb93e26fe27daf744fe1f3</anchor>
      <arglist>(const Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a41292530583857fa7d7ccfdce00fb3ba</anchor>
      <arglist>(Array&lt; char, void(*)(char *, std::size_t) &gt; &amp;&amp;) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a510ced09df574be474049fdce4f18d37</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a54d801f112a8c352841d8bec5543d9db</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a96c848506ad58fe0be0d9fd88ad0a8e6</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const void &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a9f885b5454d6afb2473f77cf8a12fa40</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator char *</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a900d45e746267f9b9aa493fd23dde5f8</anchor>
      <arglist>()&amp;</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const char *</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>acc671fef58ee49ca4771122488f0802f</anchor>
      <arglist>() const &amp;</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a0c46890d99721a859619561f719b4e31</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a3f9c22c6aae13d1facd5e62754726a30</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void(*)(char *, std::size_t)</type>
      <name>deleter</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab789c4323af5329c9e0156f6f07166fd</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a05e93170099dc30d55613b66ebd677fe</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a70adb0ac58742a67cd473cf8fc1550fe</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>abaad6108087008802cd894a6679e2b94</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa7890be6e558701eba9066f21f7cf78e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a6c878516aede0c4600ee69f37ee8e990</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a339172cd6f805a667dfbebdcefa533b9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab6d697106a65f4ad2e394db29cd5a5ba</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae40dad9a339c9b48ea0800e3d2bceb82</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a31ed9632bbdf1612906867854df485bd</anchor>
      <arglist>(char *begin, char *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a0acff85379dc67fb88daea9ae86b644d</anchor>
      <arglist>(const char *begin, const char *end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa73a1d7ad0ac1447d8d88bf4528fad53</anchor>
      <arglist>(std::size_t begin, std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aedf689d02359da0feceaa615f84f7722</anchor>
      <arglist>(std::size_t begin, std::size_t end) const</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a824ad0c5ac1722ca966e69177c136622</anchor>
      <arglist>(char *begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aab6355de230df80dfcbe7b27b29a0e97</anchor>
      <arglist>(const char *begin) const</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a3364f1140c218b837780b10b970b326f</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aaa58f440515780a1da0474d40d44e9fd</anchor>
      <arglist>(std::size_t begin) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a24be60e39728f7d05ba83b0a8eb506bf</anchor>
      <arglist>(char *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a039af1ea7c350c652a8b497748c1d1ce</anchor>
      <arglist>(const char *end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>af67d0fcf514203404db65d818d718208</anchor>
      <arglist>(std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a37857c627f00cf389662459324931cfa</anchor>
      <arglist>(std::size_t end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a803162c8189d5ce30496dd61d515036b</anchor>
      <arglist>(char *begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a88ca301aaed64e61fcac78dc3b4f0548</anchor>
      <arglist>(const char *begin) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>adf6635159fa9c6d4c47c4f98ef0248af</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a9716d3461c93e28027e45a794a14c489</anchor>
      <arglist>(std::size_t begin) const</arglist>
    </member>
    <member kind="function">
      <type>char *</type>
      <name>release</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa6256e53419003ba6f434a91f293c92e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Array&lt; char, void(*)(char *, std::size_t) &gt;</type>
      <name>from</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>af0354ec5437d70fbe0b20c5faae3d56a</anchor>
      <arglist>(U &amp;&amp;...values)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Array&lt; char, void(*)(char *, std::size_t) &gt;</type>
      <name>zeroInitialized</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a2c8b867b464b326c7c56a535bef2950c</anchor>
      <arglist>(std::size_t size)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>ArrayView&lt; const char &gt;</name>
    <filename>classCorrade_1_1Containers_1_1ArrayView.html</filename>
    <member kind="typedef">
      <type>const char</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>acffca460a3bea7a4721e765f069587b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a832bfa9a5d0aa6791d995a80e60621b6</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>aa5b6bedc075e0698b30846770d50d312</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a67f28104cc25005a20a4e1501f264336</anchor>
      <arglist>(const char *data, std::size_t size) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a4318ae04782dfafbdbbe764bc3e5f15b</anchor>
      <arglist>(U(&amp;data)[size]) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a983523ac9454f971954d89363bf044c8</anchor>
      <arglist>(ArrayView&lt; U &gt; view) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8c3a0e9e3639eb47f6c99b9d72556bf2</anchor>
      <arglist>(StaticArrayView&lt; size, U &gt; view) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ab10bd74a9f8775c74a989d458fd9a670</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator const char *</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8fdd286259e63ac1e75526e9b12fac44</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr const char *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8b9def1f69fa035a1a814da0ed6b322b</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a64de93112836f1f80e51a3994471fe24</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ae62a1deaf93bf4d382623fb62975694d</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr const char *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>aa0999ca688430fb80fcc9a7fda0dd2fa</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr const char *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a57e031ae02bb22b69342c8fba8159d8e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a73eb72e5c5cd52589d6fd0b8601200dd</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a6e9e8aa178bea52b57698c881d1a3962</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a548355d8755557b2741913860b0edf98</anchor>
      <arglist>(const char *begin, const char *end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>accd0abee2cafdfd4eff2b28b72763e56</anchor>
      <arglist>(std::size_t begin, std::size_t end) const</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>adcaaffc0c98363a2b0929c6987b41e83</anchor>
      <arglist>(const char *begin) const</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, const char &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a2a2734d614bfd48313af42cb9d5bf16d</anchor>
      <arglist>(std::size_t begin) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a993140b9bc4003a2257f6fb35e923d83</anchor>
      <arglist>(const char *end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a5c28369fabfd16556960e8a7b96ec7a9</anchor>
      <arglist>(std::size_t end) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a9dc1e8fbc43320312c00d49c1e095ec8</anchor>
      <arglist>(const char *begin) const</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const char &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a50834bc412fb57a33d72f54cef8e47ef</anchor>
      <arglist>(std::size_t begin) const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Comparator&lt; Compare::Corrade::TestSuite::Compare::Around&lt; T &gt; &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a8d072896c5b922d59acbba7e5db65613</anchor>
      <arglist>(const Compare::Corrade::TestSuite::Compare::Around&lt; T &gt; &amp;actual, const Compare::Corrade::TestSuite::Compare::Around&lt; T &gt; &amp;expected)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>printErrorMessage</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a36ee68f3f8ae16b173f92e968d2c4a5d</anchor>
      <arglist>(Utility::Error &amp;e, const std::string &amp;actual, const std::string &amp;expected) const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Comparator&lt; Compare::Corrade::TestSuite::Compare::File &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a8d072896c5b922d59acbba7e5db65613</anchor>
      <arglist>(const Compare::Corrade::TestSuite::Compare::File &amp;actual, const Compare::Corrade::TestSuite::Compare::File &amp;expected)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>printErrorMessage</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a36ee68f3f8ae16b173f92e968d2c4a5d</anchor>
      <arglist>(Utility::Error &amp;e, const std::string &amp;actual, const std::string &amp;expected) const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Comparator&lt; StringLength &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a8d072896c5b922d59acbba7e5db65613</anchor>
      <arglist>(const StringLength &amp;actual, const StringLength &amp;expected)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>printErrorMessage</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a36ee68f3f8ae16b173f92e968d2c4a5d</anchor>
      <arglist>(Utility::Error &amp;e, const std::string &amp;actual, const std::string &amp;expected) const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Container&lt; T &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1Container.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::Array</name>
    <filename>classCorrade_1_1Containers_1_1Array.html</filename>
    <templarg>T</templarg>
    <templarg>D</templarg>
    <member kind="typedef">
      <type>T</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab88bec3b842462e9eea5157a4049a094</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>D</type>
      <name>Deleter</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a6397e3aabed2ca448871a747e19a7ee7</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae5ab628ed1d689df54ee1e14c9f33e00</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab4c2451e1504971dee5076ad43300be8</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a4fcbcb24d5a462f13e7dc8e138664e31</anchor>
      <arglist>(DefaultInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ada8220cec2457b9e1730b71d24e3e60e</anchor>
      <arglist>(ValueInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ad6a7e145daae045d737003b626cc958d</anchor>
      <arglist>(NoInitT, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ad0e6ed64bc694f8e4b316848314c5345</anchor>
      <arglist>(DirectInitT, std::size_t size, Args &amp;&amp;...args)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa48105e1a6a95c402c274d32d8d213c1</anchor>
      <arglist>(InPlaceInitT, std::initializer_list&lt; T &gt; list)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a913499c6a05cd36964ad56d0eaa1f002</anchor>
      <arglist>(std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab527926a8480885ca15d7fd53c4e0cef</anchor>
      <arglist>(T *data, std::size_t size, D deleter=Implementation::DefaultDeleter&lt; D &gt;{}())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a96d5213dcca0193ecd6c8d67217efde2</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Array</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ade7103081fcb82b77f40b3a6a0c9bd1f</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type>Array&lt; T, D &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae4fd95d4d6fb93e26fe27daf744fe1f3</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Array&lt; T, D &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a41292530583857fa7d7ccfdce00fb3ba</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;&amp;) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a510ced09df574be474049fdce4f18d37</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a54d801f112a8c352841d8bec5543d9db</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a96c848506ad58fe0be0d9fd88ad0a8e6</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const void &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a9f885b5454d6afb2473f77cf8a12fa40</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a900d45e746267f9b9aa493fd23dde5f8</anchor>
      <arglist>()&amp;</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>acc671fef58ee49ca4771122488f0802f</anchor>
      <arglist>() const &amp;</arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a0c46890d99721a859619561f719b4e31</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a3f9c22c6aae13d1facd5e62754726a30</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>D</type>
      <name>deleter</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab789c4323af5329c9e0156f6f07166fd</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a05e93170099dc30d55613b66ebd677fe</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a70adb0ac58742a67cd473cf8fc1550fe</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>abaad6108087008802cd894a6679e2b94</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa7890be6e558701eba9066f21f7cf78e</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a6c878516aede0c4600ee69f37ee8e990</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a339172cd6f805a667dfbebdcefa533b9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ab6d697106a65f4ad2e394db29cd5a5ba</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ae40dad9a339c9b48ea0800e3d2bceb82</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a31ed9632bbdf1612906867854df485bd</anchor>
      <arglist>(T *begin, T *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a0acff85379dc67fb88daea9ae86b644d</anchor>
      <arglist>(const T *begin, const T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa73a1d7ad0ac1447d8d88bf4528fad53</anchor>
      <arglist>(std::size_t begin, std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aedf689d02359da0feceaa615f84f7722</anchor>
      <arglist>(std::size_t begin, std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a824ad0c5ac1722ca966e69177c136622</anchor>
      <arglist>(T *begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aab6355de230df80dfcbe7b27b29a0e97</anchor>
      <arglist>(const T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a3364f1140c218b837780b10b970b326f</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size, const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aaa58f440515780a1da0474d40d44e9fd</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a24be60e39728f7d05ba83b0a8eb506bf</anchor>
      <arglist>(T *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a039af1ea7c350c652a8b497748c1d1ce</anchor>
      <arglist>(const T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>af67d0fcf514203404db65d818d718208</anchor>
      <arglist>(std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a37857c627f00cf389662459324931cfa</anchor>
      <arglist>(std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a803162c8189d5ce30496dd61d515036b</anchor>
      <arglist>(T *begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a88ca301aaed64e61fcac78dc3b4f0548</anchor>
      <arglist>(const T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>adf6635159fa9c6d4c47c4f98ef0248af</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a9716d3461c93e28027e45a794a14c489</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>release</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>aa6256e53419003ba6f434a91f293c92e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Array&lt; T, D &gt;</type>
      <name>from</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>af0354ec5437d70fbe0b20c5faae3d56a</anchor>
      <arglist>(U &amp;&amp;...values)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Array&lt; T, D &gt;</type>
      <name>zeroInitialized</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>a2c8b867b464b326c7c56a535bef2950c</anchor>
      <arglist>(std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>ad5f0615634b2553b0cb304aa58da3347</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>classCorrade_1_1Containers_1_1Array.html</anchorfile>
      <anchor>af8a4e6b17dca34a5eb82b6a8da8c398b</anchor>
      <arglist>(const Array&lt; T &gt; &amp;view)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::ArrayView</name>
    <filename>classCorrade_1_1Containers_1_1ArrayView.html</filename>
    <templarg>T</templarg>
    <member kind="typedef">
      <type>T</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>acffca460a3bea7a4721e765f069587b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a832bfa9a5d0aa6791d995a80e60621b6</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>aa5b6bedc075e0698b30846770d50d312</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a67f28104cc25005a20a4e1501f264336</anchor>
      <arglist>(T *data, std::size_t size) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a4318ae04782dfafbdbbe764bc3e5f15b</anchor>
      <arglist>(U(&amp;data)[size]) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a983523ac9454f971954d89363bf044c8</anchor>
      <arglist>(ArrayView&lt; U &gt; view) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8c3a0e9e3639eb47f6c99b9d72556bf2</anchor>
      <arglist>(StaticArrayView&lt; size, U &gt; view) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ab10bd74a9f8775c74a989d458fd9a670</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8fdd286259e63ac1e75526e9b12fac44</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8b9def1f69fa035a1a814da0ed6b322b</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a64de93112836f1f80e51a3994471fe24</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ae62a1deaf93bf4d382623fb62975694d</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>aa0999ca688430fb80fcc9a7fda0dd2fa</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a57e031ae02bb22b69342c8fba8159d8e</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a73eb72e5c5cd52589d6fd0b8601200dd</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a6e9e8aa178bea52b57698c881d1a3962</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a548355d8755557b2741913860b0edf98</anchor>
      <arglist>(T *begin, T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>accd0abee2cafdfd4eff2b28b72763e56</anchor>
      <arglist>(std::size_t begin, std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>adcaaffc0c98363a2b0929c6987b41e83</anchor>
      <arglist>(T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a2a2734d614bfd48313af42cb9d5bf16d</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a993140b9bc4003a2257f6fb35e923d83</anchor>
      <arglist>(T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a5c28369fabfd16556960e8a7b96ec7a9</anchor>
      <arglist>(std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a9dc1e8fbc43320312c00d49c1e095ec8</anchor>
      <arglist>(T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a50834bc412fb57a33d72f54cef8e47ef</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a57f8d4e0a25951f539461539c90d4e25</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a4258433529f847d3e6ed10442f6b9b8b</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a32ef8daa386aee742e5f39fc5b1516fd</anchor>
      <arglist>(T *data, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ac8edb9cd0a74f5a0da2ef9c6d03f9ef2</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>ac450a49515ff20058552c732542b1b13</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a8e6435208f2cda38505084a930a3e9ec</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView.html</anchorfile>
      <anchor>a00abca03029b326133e9e7c908508af4</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::ArrayView&lt; const void &gt;</name>
    <filename>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</filename>
    <member kind="typedef">
      <type>const void</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a866e1baad53839e2c343d7086735737a</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a0a6b855a35b55a914ceb39bee62bdb39</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a2dff420240c3c957e066de9d446ab0d4</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a4b52d12c269029373ae89057494cac6a</anchor>
      <arglist>(const void *data, std::size_t size) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a6d3439e67990086c4602766e98da45fc</anchor>
      <arglist>(const T *data, std::size_t size) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a706d201f881c928c380395efe8828c77</anchor>
      <arglist>(T(&amp;data)[size]) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a94b3ec643185461eb3df3d5ffe83be61</anchor>
      <arglist>(const ArrayView&lt; T &gt; &amp;array) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>ArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a37da08f38dda6d06dc36196dbbc6f29a</anchor>
      <arglist>(const StaticArrayView&lt; size, T &gt; &amp;array) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a2e085c9c6f4fb239bd7ac652267a5ead</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator const void *</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a3b98a3b73d6cee67dd23b4e471957faf</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr const void *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>a345c0acd3e36825e4eeff227c8d804e5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>aec97e829a34464332bc88ead24c42226</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1ArrayView_3_01const_01void_01_4.html</anchorfile>
      <anchor>af8f676cd3a40a843731b71cdba2c5de9</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>Corrade::Containers::DefaultInitT</name>
    <filename>structCorrade_1_1Containers_1_1DefaultInitT.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Containers::DirectInitT</name>
    <filename>structCorrade_1_1Containers_1_1DirectInitT.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::EnumSet</name>
    <filename>classCorrade_1_1Containers_1_1EnumSet.html</filename>
    <templarg>T</templarg>
    <templarg>fullValue</templarg>
    <member kind="enumvalue">
      <name>FullValue</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>aee87dfd8eec50c26f5f9703feea66abda9e456595199d05472bf7927d329639b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>T</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>af79d534425b3439095c579944858bb4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>std::underlying_type&lt; T &gt;::type</type>
      <name>UnderlyingType</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ae98013f13d62ca0bac69d632905846c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FullValue</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>aee87dfd8eec50c26f5f9703feea66abda9e456595199d05472bf7927d329639b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ac3820645815d705507170a3e507087b5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>aff6e8c4b80a660f381b31a2ed5106695</anchor>
      <arglist>(T value)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>adb0d3bd1925b399503dc745e51c245a6</anchor>
      <arglist>(NoInitT)</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator==</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a46221489ad6521358ffcf7845f9d3462</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator!=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a7cb7f4bd84490b718800c3ba2f1df17e</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator&gt;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a3885d6aa88442661407ee1a9ecb86ec3</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator&lt;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a42906634012570f68eb709b6c5774053</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; T, fullValue &gt;</type>
      <name>operator|</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ac6c4a74c4c567c7b5b6174c50203113a</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; T, fullValue &gt; &amp;</type>
      <name>operator|=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a0570b5800f24317a1b52b27d878b9bfa</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; T, fullValue &gt;</type>
      <name>operator&amp;</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a61cf465a2d9cb28bcb3a139f6db20f98</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; T, fullValue &gt; &amp;</type>
      <name>operator&amp;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a882bec7925638c92d8491ce07108f398</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; T, fullValue &gt;</type>
      <name>operator^</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a6cf9b501b4fb1c5c83b900defc7d2ebf</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other) const </arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; T, fullValue &gt; &amp;</type>
      <name>operator^=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>afc5caf6f9260aedc01b5d532e4143e36</anchor>
      <arglist>(EnumSet&lt; T, fullValue &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; T, fullValue &gt;</type>
      <name>operator~</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a37f0da0636b0e7fbca8a4f3ed7c59c9c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a999c11f444c7589b574ea80e9087c1d9</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator UnderlyingType</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>abb5a6545e21fd33af55c6d20a0e3277f</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Utility::Debug &amp;</type>
      <name>enumSetDebugOutput</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a12c4ea794fa62c56969b1311e2b5c21b</anchor>
      <arglist>(Utility::Debug &amp;debug, EnumSet&lt; T, fullValue &gt; value, const char *empty, std::initializer_list&lt; T &gt; enums)</arglist>
    </member>
    <docanchor file="classCorrade_1_1Containers_1_1EnumSet">EnumSet-out-of-class-operators</docanchor>
    <docanchor file="classCorrade_1_1Containers_1_1EnumSet">EnumSet-friend-operators</docanchor>
  </compound>
  <compound kind="struct">
    <name>Corrade::Containers::InPlaceInitT</name>
    <filename>structCorrade_1_1Containers_1_1InPlaceInitT.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::LinkedList</name>
    <filename>classCorrade_1_1Containers_1_1LinkedList.html</filename>
    <templarg>T</templarg>
    <member kind="function">
      <type>constexpr</type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>aef8b2c6fac22b76d6510b030de0bd7ff</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ae30e2ad2d45ed813d5b6517da99c2c45</anchor>
      <arglist>(const LinkedList&lt; T &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a20a700a56461d5b8a6892ca147384609</anchor>
      <arglist>(LinkedList&lt; T &gt; &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ab9bdae1a80b28b53f4b70edd139c2741</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>LinkedList&lt; T &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a93fe27abbdca0a2bdfffbb0c1c9e52b2</anchor>
      <arglist>(const LinkedList&lt; T &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>LinkedList&lt; T &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a77a0c2633a85000db72a6acf84cfde45</anchor>
      <arglist>(LinkedList&lt; T &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>first</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a9d49452ca29d53f6313285e8426ec13a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr const T *</type>
      <name>first</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a7bd1e169057986db30b9b54da8bae08c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>last</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>abdb5e04bfec6bde39d8a20dd8d4d4674</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr const T *</type>
      <name>last</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a62c2d194213d6af1702da006769f2f8a</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>isEmpty</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a44ca290841e84a0df958a797eea3f17d</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>insert</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ab85682283d7ef74f20a2c59a3153f9a1</anchor>
      <arglist>(T *item, T *before=nullptr)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>cut</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a43d45cf92d3f88f450047b773e5fe157</anchor>
      <arglist>(T *item)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>move</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a39ec134725e0b19f1d9bdfcd5e807973</anchor>
      <arglist>(T *item, T *before)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>erase</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a5d42a873dd83fbc0cc0084209536f9be</anchor>
      <arglist>(T *item)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clear</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a83550122caae33c90d11d45d0b747184</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::LinkedListItem</name>
    <filename>classCorrade_1_1Containers_1_1LinkedListItem.html</filename>
    <templarg>Derived</templarg>
    <templarg>List</templarg>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a61ce16db8160e1d80e881d5761f4752b</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>abe1d3f91dd60c7f44e177f6b4c06c016</anchor>
      <arglist>(const LinkedListItem&lt; Derived, List &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a5a9b570f0fe78f27598febccdae1ddde</anchor>
      <arglist>(LinkedListItem&lt; Derived, List &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>LinkedListItem &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>acd27fdb8bced1051b1a0ea2c8f866f18</anchor>
      <arglist>(const LinkedListItem&lt; Derived, List &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>LinkedListItem&lt; Derived, List &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>af9f41d1506c9ef6ef4e4f0bb8f3882ef</anchor>
      <arglist>(LinkedListItem&lt; Derived, List &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual</type>
      <name>~LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a0863b9d8b2ea61e35ff0aac3780870cb</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function">
      <type>List *</type>
      <name>list</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>ab607f167b90d2b00e42b238240afb36d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const List *</type>
      <name>list</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>aeb0b84fe7899d4229b1f229d095d6ccd</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Derived *</type>
      <name>previous</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a600d8bd1ebea7689159d9c37e1302fc4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const Derived *</type>
      <name>previous</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a3dff4a6d49d907850860f9077f7bd5ce</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Derived *</type>
      <name>next</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>ac878e7e23048b57e2e577f73ff2d4870</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const Derived *</type>
      <name>next</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a31c7f69f017a70ddcdf6ae8a9bbce24a</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>Corrade::Containers::NoInitT</name>
    <filename>structCorrade_1_1Containers_1_1NoInitT.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::StaticArray</name>
    <filename>classCorrade_1_1Containers_1_1StaticArray.html</filename>
    <templarg>size_</templarg>
    <templarg></templarg>
    <member kind="enumvalue">
      <name>Size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a66a7fd8773137497eb0746e37658ab4aa793db039a8e75b1828c2a507e5838126</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>T</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ad9b13acda5c9963c7e36f7d94f225655</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a66a7fd8773137497eb0746e37658ab4aa793db039a8e75b1828c2a507e5838126</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a08d107eecec22bd96e22d9fb5d8297b2</anchor>
      <arglist>(DefaultInitT)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a1157493c2427106a49cc1ca01b1ef3b8</anchor>
      <arglist>(ValueInitT)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>aae4233fd0137914175919794303ee9a5</anchor>
      <arglist>(NoInitT)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>adae37e3fc8052374d238e538ec336bc0</anchor>
      <arglist>(DirectInitT, Args &amp;&amp;...args)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ad942c7693e64e00abdf37016b2c20533</anchor>
      <arglist>(InPlaceInitT, Args &amp;&amp;...args)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ac41f170965520c13b1b1c9d07c919a7a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a8ea22f6feba81dafb6ed6b436a617a7d</anchor>
      <arglist>(Args &amp;&amp;...args) StaticArray(const StaticArray&lt; size_</arglist>
    </member>
    <member kind="function">
      <type>T &amp;</type>
      <name>StaticArray</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ac13ce18335a47a6ed25e26ac45e2c0b5</anchor>
      <arglist>(StaticArray&lt; size_, T &gt; &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>StaticArray&lt; size_, T &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a582c622e6472848f441e482dca40c0ad</anchor>
      <arglist>(const StaticArray&lt; size_, T &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>StaticArray&lt; size_, T &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ab2b8392b6ccec16fe2694f48ae584b40</anchor>
      <arglist>(StaticArray&lt; size_, T &gt; &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>afc823e6a5a355414cc8bef2cdb6be583</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a5491de858b12f8f5e5170d6c6c7524b2</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator ArrayView&lt; const void &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ad9097dc3b8e766807d15f55e2cac68aa</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator StaticArrayView&lt; size_, U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a4f1d81354b1d8b0d57710ed907b50b36</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator StaticArrayView&lt; size_, const U &gt;</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>af196863391459b7ebe732afefb7f50d3</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>af02c1b9200608ae9e0fe286d4bf356fe</anchor>
      <arglist>()&amp;</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator const T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a9672861f978d238efea9bef3ae9aa8bf</anchor>
      <arglist>() const &amp;</arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ad36bd00f178baa6fcb1dfc8f2b2461fb</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>acbab8c8514ed0fced5141bcd37c08ae3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>aa4eb922f8f11f0d8feceeec0453a47d5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a70a1f45ab56ac5b4783d3a873156b070</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a1ae548f4a2509c8c1c4a25a02075e52c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a5c4d26d24fe9a16c42f3825d2ab56f1e</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a49f2a6817b5875db75c83dfe8013c82b</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a5c5506da630022bfa4a92daaadf5d5d4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a9c2bb413bda466e1f9cce0b9092352f3</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const T *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ae2f147ed2711ed0d0b0d8a63da338864</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a79930f8ca1d65cbe71976589adc5e367</anchor>
      <arglist>(T *begin, T *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a9ae7916eb24d16e77096fc28a3ef4903</anchor>
      <arglist>(const T *begin, const T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a2f5a20be2e89d05cddc0043e204fd4d2</anchor>
      <arglist>(std::size_t begin, std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>acaf7ae87687cc8aa63ce7590ebe6f2a5</anchor>
      <arglist>(std::size_t begin, std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>aafd695442305eac55f636b1905988d13</anchor>
      <arglist>(T *begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a14ab03bbb3c2f267a4ae6995e1036aaa</anchor>
      <arglist>(const T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a3405a0f3f83ec4947ac58e56f2388482</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, const T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>af7094cef42f7e7ba7adb8df7e94f775d</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a47fc19252e6719d0d53cb80c70523875</anchor>
      <arglist>(T *end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ae2b72369f92acd5fd8bdca2af364939a</anchor>
      <arglist>(const T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ad7042c5d4d41d36f37234419f42bd93e</anchor>
      <arglist>(std::size_t end)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>afaaa2ea289989304706ea0b16d891da9</anchor>
      <arglist>(std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>aac621f1a78ed76071622d73a584d69a3</anchor>
      <arglist>(T *begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a47708e8f28ebbdd3ea7260a3fd91e994</anchor>
      <arglist>(const T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a886b160fc0e823ac4542eb1ad4ca63c6</anchor>
      <arglist>(std::size_t begin)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a027333a40c0da5e055b087eff52afd12</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>aa8111199cdb6cc30c470e3cc9c54a30e</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a5b40a450ade4927381bf97a165e3621e</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a4f6c86de70a1ab496a1d798648c22f80</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, const T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>ab15dc352f75f85e562eaa85ae058ac9f</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a5ebf447d7f388e81fe6d34910c80cfbe</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArray.html</anchorfile>
      <anchor>a0cad6e09b65666040bb2460df883e4a3</anchor>
      <arglist>(const StaticArray&lt; size_, T &gt; &amp;)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Containers::StaticArrayView</name>
    <filename>classCorrade_1_1Containers_1_1StaticArrayView.html</filename>
    <templarg>size_</templarg>
    <templarg></templarg>
    <member kind="enumvalue">
      <name>Size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a81a453242bb81aaf4602bb73b29280d5a50dc475b85658e4e449517955ac6c5ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>T</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a79d34b8f42168ee332f42fd462dfbbc2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a81a453242bb81aaf4602bb73b29280d5a50dc475b85658e4e449517955ac6c5ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>StaticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>afa86e55be630568e83e24f1ef212ffa7</anchor>
      <arglist>(std::nullptr_t) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>StaticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>aef1b8c9af2177bf7e0272755ae5cd26f</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>StaticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a72c71c4efe6a687e1d0f5165b2100466</anchor>
      <arglist>(T *data) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>StaticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a591e233d706fa154872a7eae4d949193</anchor>
      <arglist>(U(&amp;data)[size_]) noexcept</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>StaticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a2edead72560a51aa054885027f2b56c7</anchor>
      <arglist>(StaticArrayView&lt; size_, U &gt; view) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a8426ff4e4878190d5e9c20a238600f95</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator T *</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a1bf4246b1100ddf3ec063e8ab50e29c5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>aeddfe06ecc1a107c49091c7c77cb42b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>size</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a4be86f82dcaac36c5f8a204c5d4060ad</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>empty</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a10382f3d57ce2c829a159baeea80a448</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>begin</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a258d0692f28d3902420ce177e6ce2e82</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr T *</type>
      <name>cbegin</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>ad0f049ff52182f96e2bedc9e483cff0c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>end</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>ae368f97599e45bf9b7d29f46f56af180</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T *</type>
      <name>cend</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a6de6684c2b189aea9a11b3a82a002193</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a6744528a26574f7602dbad9bbe28f858</anchor>
      <arglist>(T *begin, T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>ae19a20c5b57af6b15f4066b143ee6fbe</anchor>
      <arglist>(std::size_t begin, std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>ad6b92915ee74c4d8a5fa3a391ff95a96</anchor>
      <arglist>(T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; viewSize, T &gt;</type>
      <name>slice</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>ae7b350017afcfff13e06c5d812778b91</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a1b0835029d0063a394f7832ddb920aac</anchor>
      <arglist>(T *end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>prefix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a6cadf010bfd2ff4341ac5522052827ce</anchor>
      <arglist>(std::size_t end) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a36e3f9d44c1398d126ef83aeb5aefd5f</anchor>
      <arglist>(T *begin) const </arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>suffix</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a50ebc3f3897a280a28ed2e239288c09f</anchor>
      <arglist>(std::size_t begin) const </arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a58d7bb8d46f44949b57b226485db4a9b</anchor>
      <arglist>(T *data)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>a3cf428b256a5ed8941145e4d15ae8b45</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>aee41b58bdb45963c476ce7554dda2290</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>classCorrade_1_1Containers_1_1StaticArrayView.html</anchorfile>
      <anchor>aee9c43d476128f05403e638ce539c90d</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>Corrade::Containers::ValueInitT</name>
    <filename>structCorrade_1_1Containers_1_1ValueInitT.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Filesystem::AbstractFilesystem</name>
    <filename>classCorrade_1_1Filesystem_1_1AbstractFilesystem.html</filename>
    <base>Corrade::PluginManager::AbstractPlugin</base>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::Connection</name>
    <filename>classCorrade_1_1Interconnect_1_1Connection.html</filename>
    <member kind="function">
      <type></type>
      <name>Connection</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a908fe313b0f50d7bf05e5444401ba9b6</anchor>
      <arglist>(const Connection &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Connection</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>afb2bf12cfd5fb26620804385d968e0be</anchor>
      <arglist>(Connection &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type>Connection &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a86b6f3654f768080d7963946ca3d92ec</anchor>
      <arglist>(const Connection &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Connection &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a69d16e723cddffc382491b8b485f0b24</anchor>
      <arglist>(Connection &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~Connection</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>aa27cc7bc4e2951f4b303c1a788c8b432</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isConnectionPossible</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a1cd23e0584fe131a6f6bc295eb4532d8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isConnected</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a945e791b882c1092a2da42643a1c8e03</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>connect</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>a2da3a80eb849dfd30148aa643efc6b70</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>disconnect</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Connection.html</anchorfile>
      <anchor>aaf5525583e6564580dcec30122470f13</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::Emitter</name>
    <filename>classCorrade_1_1Interconnect_1_1Emitter.html</filename>
    <class kind="class">Corrade::Interconnect::Emitter::Signal</class>
    <member kind="function">
      <type></type>
      <name>Emitter</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>acb307bb054059a1b4faff4ced7df8b14</anchor>
      <arglist>(const Emitter &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Emitter</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a65e30e6dc9875908864c42fbebe31ecc</anchor>
      <arglist>(Emitter &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Emitter &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a5da79d646791d792941ad516b0fd3205</anchor>
      <arglist>(const Emitter &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Emitter &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a722e415daa51bdba50cf73a8e00838fb</anchor>
      <arglist>(Emitter &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasSignalConnections</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a372cbddd6bb7cbe8b593c7f4955f2caf</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasSignalConnections</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>aa003c590e47cdc58f9b49587c195b95f</anchor>
      <arglist>(Signal(Emitter::*signal)(Args...)) const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>signalConnectionCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>af05b3494cac10d0fc846ec484337c47e</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>signalConnectionCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>ad144bb3d36a1199c8f9ee8263497695c</anchor>
      <arglist>(Signal(Emitter::*signal)(Args...)) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>disconnectSignal</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a5139e1e243d19690de8412be122ce37a</anchor>
      <arglist>(Signal(Emitter::*signal)(Args...))</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>disconnectAllSignals</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>afff8419f54c355125c47f631373b9b4e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Signal</type>
      <name>emit</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a9b6202322d7683b9f7655157b45528ff</anchor>
      <arglist>(Signal(Emitter::*signal)(Args...), typename std::common_type&lt; Args &gt;::type...args)</arglist>
    </member>
    <member kind="friend" protection="private">
      <type>friend Connection</type>
      <name>connect</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a07fd3fa65312056cf5d4d6eeec9af7ed</anchor>
      <arglist>(EmitterObject &amp;, Signal(Emitter::*)(Args...), ReceiverObject &amp;, void(Receiver::*)(Args...))</arglist>
    </member>
    <member kind="friend" protection="private">
      <type>friend Connection</type>
      <name>connect</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Emitter.html</anchorfile>
      <anchor>a51ccdf5d8394169814736c891c740508</anchor>
      <arglist>(EmitterObject &amp;, Signal(Emitter::*)(Args...), void(*)(Args...))</arglist>
    </member>
    <docanchor file="classCorrade_1_1Interconnect_1_1Emitter">Interconnect-Emitter-signals</docanchor>
    <docanchor file="classCorrade_1_1Interconnect_1_1Emitter">Interconnect-Emitter-connections</docanchor>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::Emitter::Signal</name>
    <filename>classCorrade_1_1Interconnect_1_1Emitter_1_1Signal.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::Receiver</name>
    <filename>classCorrade_1_1Interconnect_1_1Receiver.html</filename>
    <member kind="function">
      <type></type>
      <name>Receiver</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>a57b82b81813abb257b3c31dc7dd461f3</anchor>
      <arglist>(const Receiver &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Receiver</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>adbe214ae7135e06e2dfdcffa5a5702e4</anchor>
      <arglist>(Receiver &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Receiver &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>aa0a89c026f6d28aa81158697cf949fb0</anchor>
      <arglist>(const Receiver &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Receiver &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>a4d8747f9488cf8d87e31f4e4b64c436e</anchor>
      <arglist>(Receiver &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasSlotConnections</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>ad456682f8f89648dd263f91c758a40f8</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>slotConnectionCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>a8253840b00b7e3ee1d8568906b347683</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>disconnectAllSlots</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1Receiver.html</anchorfile>
      <anchor>aa16eeb8b069f5008586b8f68ccc6dd7f</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::StateMachine</name>
    <filename>classCorrade_1_1Interconnect_1_1StateMachine.html</filename>
    <templarg>states</templarg>
    <templarg>inputs</templarg>
    <templarg></templarg>
    <templarg></templarg>
    <base>Corrade::Interconnect::Emitter</base>
    <member kind="enumvalue">
      <name>StateCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a27f485964e3e6a67b0c44cd246908b00ad07f6229f97a406202285d31f475db70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>InputCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a27f485964e3e6a67b0c44cd246908b00a269ea6d765c72e02fd88ac70c3067847</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>StateCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a27f485964e3e6a67b0c44cd246908b00ad07f6229f97a406202285d31f475db70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>InputCount</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a27f485964e3e6a67b0c44cd246908b00a269ea6d765c72e02fd88ac70c3067847</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>StateMachine</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a1632034558adf149fe2cdee82d44af65</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>State</type>
      <name>current</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>ae8b3a8090f9b34f4777dd3a85a11298a</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addTransitions</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>ab331b1128a353a33a58420f68392d417</anchor>
      <arglist>(std::initializer_list&lt; StateTransition&lt; State, Input &gt;&gt; transitions)</arglist>
    </member>
    <member kind="function">
      <type>StateMachine&lt; states, inputs, State, Input &gt; &amp;</type>
      <name>step</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a659a4bc7dc0220dafe101cc9314543b8</anchor>
      <arglist>(Input input)</arglist>
    </member>
    <member kind="function">
      <type>Signal</type>
      <name>stepped</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>aad2ec34f5cfc02298bf7591fdb2cf2e6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Signal</type>
      <name>entered</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a2d038d607bcaddc5777c08c655244b51</anchor>
      <arglist>(State previous)</arglist>
    </member>
    <member kind="function">
      <type>Signal</type>
      <name>exited</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateMachine.html</anchorfile>
      <anchor>a8439d25865bb82eeb0ff38ae8c27c82e</anchor>
      <arglist>(State next)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Interconnect::StateTransition</name>
    <filename>classCorrade_1_1Interconnect_1_1StateTransition.html</filename>
    <templarg></templarg>
    <templarg></templarg>
    <member kind="function">
      <type>constexpr</type>
      <name>StateTransition</name>
      <anchorfile>classCorrade_1_1Interconnect_1_1StateTransition.html</anchorfile>
      <anchor>a2e1b5ba516ad502534fd93dcc716fefe</anchor>
      <arglist>(State from, Input input, State to)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::PluginManager::AbstractManager</name>
    <filename>classCorrade_1_1PluginManager_1_1AbstractManager.html</filename>
    <member kind="function">
      <type></type>
      <name>AbstractManager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a15c48240332b27b1066308f2e08e2857</anchor>
      <arglist>(const AbstractManager &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractManager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>ac897a1c0583f0f251b69c47ced65a960</anchor>
      <arglist>(AbstractManager &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>AbstractManager &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>ad1e9417feb5fd524d9f59daaf3c6e1cf</anchor>
      <arglist>(const AbstractManager &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>AbstractManager &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a498d594eb9f1e99325a1ac2c3c372859</anchor>
      <arglist>(AbstractManager &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>pluginInterface</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a4698b04061854b4c0e8a29958206cc4a</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>pluginDirectory</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a92f048279d7ac78006eb8c27f537230c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setPluginDirectory</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a35435cec6070fc9384c7baea2f79723e</anchor>
      <arglist>(std::string directory)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reloadPluginDirectory</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>afbcdc551e12ded49ee9f709d1f55ddeb</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>pluginList</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a52bd2c0a000f6695cbd586cc54355fee</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const PluginMetadata *</type>
      <name>metadata</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a2b398c5fd801067fac46ca7488c00390</anchor>
      <arglist>(const std::string &amp;plugin) const </arglist>
    </member>
    <member kind="function">
      <type>LoadState</type>
      <name>loadState</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>afdd621884dc8fa4fa0da7e9428cb3383</anchor>
      <arglist>(const std::string &amp;plugin) const </arglist>
    </member>
    <member kind="function">
      <type>LoadState</type>
      <name>load</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>ae9b3cb96444fb9b5eadccf2b368e071b</anchor>
      <arglist>(const std::string &amp;plugin)</arglist>
    </member>
    <member kind="function">
      <type>LoadState</type>
      <name>unload</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>aad58623cdf43ba4bf439d683bdc0fed3</anchor>
      <arglist>(const std::string &amp;plugin)</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const int</type>
      <name>Version</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a4d34e4baaa7d38999531ea49ad85cf26</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>~AbstractManager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManager.html</anchorfile>
      <anchor>a8ff98135f8b6356a26732fe0c9c34f10</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::PluginManager::AbstractManagingPlugin</name>
    <filename>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</filename>
    <templarg></templarg>
    <base>Corrade::PluginManager::AbstractPlugin</base>
    <member kind="function">
      <type></type>
      <name>AbstractManagingPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</anchorfile>
      <anchor>a7126dbd19b67dacdeb12c61f0543ef67</anchor>
      <arglist>()=default</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractManagingPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</anchorfile>
      <anchor>a4e2b38042c06d11a3ae4b6c5e7ac8a30</anchor>
      <arglist>(Manager&lt; Interface &gt; &amp;manager)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractManagingPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</anchorfile>
      <anchor>ab2ff51df35c509180151d970f8ea08f5</anchor>
      <arglist>(AbstractManager &amp;manager, const std::string &amp;plugin)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>Manager&lt; Interface &gt; *</type>
      <name>manager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</anchorfile>
      <anchor>afeabe42c12f4e6cdc074d4b83a81d91a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>const Manager&lt; Interface &gt; *</type>
      <name>manager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractManagingPlugin.html</anchorfile>
      <anchor>af6a4ac36467b4942fd093a4f37525adc</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::PluginManager::AbstractPlugin</name>
    <filename>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</filename>
    <member kind="function">
      <type></type>
      <name>AbstractPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a18b88d542a28a5721a708e489a1a6750</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>aaf2fbb0914f222052d0705ef5771d530</anchor>
      <arglist>(AbstractManager &amp;manager, const std::string &amp;plugin)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>af5a43e817f57f6a2c47cdb38855bce5c</anchor>
      <arglist>(const AbstractPlugin &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>AbstractPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>af04b37ff831cf928ffc54244cd8a95f9</anchor>
      <arglist>(AbstractPlugin &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual</type>
      <name>~AbstractPlugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>aabb3f885f8cb64f3d2dd0b1886d7816a</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function">
      <type>AbstractPlugin &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>aa542b4972a06b2cc0a16f5c259687475</anchor>
      <arglist>(const AbstractPlugin &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>AbstractPlugin &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a79f411129d16aeb85cec7b2a757fbdb6</anchor>
      <arglist>(AbstractPlugin &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>canBeDeleted</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a2acd3abb80cf4bb96582cca5c6670738</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const std::string &amp;</type>
      <name>plugin</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a40e78815a489268ace1271a13052c5f9</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const PluginMetadata *</type>
      <name>metadata</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a12273c7c387ef6db25d34bd70b9536db</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>initialize</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>abc4332a41668303e35c99f665983e356</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>finalize</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1AbstractPlugin.html</anchorfile>
      <anchor>a161dadb17ef0e4d0f5169b8fddb81d52</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::PluginManager::Manager</name>
    <filename>classCorrade_1_1PluginManager_1_1Manager.html</filename>
    <templarg></templarg>
    <base>Corrade::PluginManager::AbstractManager</base>
    <member kind="function">
      <type></type>
      <name>Manager</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1Manager.html</anchorfile>
      <anchor>a5fa8dd9cecbe01640da7311fcebc0e4c</anchor>
      <arglist>(std::string pluginDirectory={})</arglist>
    </member>
    <member kind="function">
      <type>std::unique_ptr&lt; T &gt;</type>
      <name>instance</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1Manager.html</anchorfile>
      <anchor>ad5bd99c59c93571ce37612eea1648460</anchor>
      <arglist>(const std::string &amp;plugin)</arglist>
    </member>
    <member kind="function">
      <type>std::unique_ptr&lt; T &gt;</type>
      <name>loadAndInstantiate</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1Manager.html</anchorfile>
      <anchor>a1b831c65abd0581f9d336bd3ad820058</anchor>
      <arglist>(const std::string &amp;plugin)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::PluginManager::PluginMetadata</name>
    <filename>classCorrade_1_1PluginManager_1_1PluginMetadata.html</filename>
    <member kind="function">
      <type>std::string</type>
      <name>name</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1PluginMetadata.html</anchorfile>
      <anchor>a348f82d7049db4655bd46f0aa1d547ad</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const std::vector&lt; std::string &gt; &amp;</type>
      <name>depends</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1PluginMetadata.html</anchorfile>
      <anchor>a16e6d759ec821a328dab1b2624bc6948</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>usedBy</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1PluginMetadata.html</anchorfile>
      <anchor>a2a645cd5165b2e79d684fb8ac7aee633</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const std::vector&lt; std::string &gt; &amp;</type>
      <name>provides</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1PluginMetadata.html</anchorfile>
      <anchor>a2a059aa3034e035015b5bcad2cfc6963</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const Utility::ConfigurationGroup &amp;</type>
      <name>data</name>
      <anchorfile>classCorrade_1_1PluginManager_1_1PluginMetadata.html</anchorfile>
      <anchor>ae998a6ccb557313441f65b7b07dade01</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Comparator</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator.html</filename>
    <templarg>T</templarg>
    <member kind="function">
      <type>bool</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a8d072896c5b922d59acbba7e5db65613</anchor>
      <arglist>(const T &amp;actual, const T &amp;expected)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>printErrorMessage</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Comparator.html</anchorfile>
      <anchor>a36ee68f3f8ae16b173f92e968d2c4a5d</anchor>
      <arglist>(Utility::Error &amp;e, const std::string &amp;actual, const std::string &amp;expected) const </arglist>
    </member>
    <docanchor file="classCorrade_1_1TestSuite_1_1Comparator">TestSuite-Comparator-pseudo-types</docanchor>
    <docanchor file="classCorrade_1_1TestSuite_1_1Comparator">TestSuite-Comparator-parameters</docanchor>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Comparator&lt; double &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator_3_01double_01_4.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Comparator&lt; float &gt;</name>
    <filename>classCorrade_1_1TestSuite_1_1Comparator_3_01float_01_4.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::Around</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1Around.html</filename>
    <templarg></templarg>
    <member kind="function">
      <type></type>
      <name>Around</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Compare_1_1Around.html</anchorfile>
      <anchor>a32bc31da967fea727ba4ef4904ee4cf4</anchor>
      <arglist>(T epsilon)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::Container</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1Container.html</filename>
    <templarg></templarg>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::File</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1File.html</filename>
    <member kind="function">
      <type></type>
      <name>File</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Compare_1_1File.html</anchorfile>
      <anchor>ab8b861a7787d2e74693a515a14197388</anchor>
      <arglist>(const std::string &amp;pathPrefix={})</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::FileToString</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1FileToString.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::Greater</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1Greater.html</filename>
    <templarg></templarg>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::GreaterOrEqual</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1GreaterOrEqual.html</filename>
    <templarg></templarg>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::Less</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1Less.html</filename>
    <templarg></templarg>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::LessOrEqual</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1LessOrEqual.html</filename>
    <templarg></templarg>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::SortedContainer</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1SortedContainer.html</filename>
    <templarg></templarg>
    <base>Container&lt; T &gt;</base>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Compare::StringToFile</name>
    <filename>classCorrade_1_1TestSuite_1_1Compare_1_1StringToFile.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Tester</name>
    <filename>classCorrade_1_1TestSuite_1_1Tester.html</filename>
    <class kind="class">Corrade::TestSuite::Tester::TesterConfiguration</class>
    <member kind="enumeration">
      <type></type>
      <name>BenchmarkType</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>aded4a627bc1573996e981425775f637d</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="aded4a627bc1573996e981425775f637da7a1920d61156abc05a60135aefe8bc67">Default</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="aded4a627bc1573996e981425775f637da75fa4d137ea5adb50f81fb3c26229797">WallTime</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="aded4a627bc1573996e981425775f637da15c5bec425f9f89b6ffdd171d67486b4">WallClock</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="aded4a627bc1573996e981425775f637da582bcd601669f4956f55cb20feb2614f">CpuTime</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="aded4a627bc1573996e981425775f637daba0d4b831fbcfdb7788fdfb4e8789207">CpuCycles</enumvalue>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>BenchmarkUnits</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a37efcdd6ea7a7ec4a8e4b304218308b4</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4afba00bdab687ce01136a86bac8bac578">Nanoseconds</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4aa76d4ef5f3f6a672bbfab2865563e530">Time</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4ad3240659659cbfa93d781d1510717a66">Cycles</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4a49cc8e6220245b65cd7d20fc6ccc74f5">Instructions</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4a600e754f49b68aa0fc90a9cd64eb7051">Bytes</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4a4789f23283b3a61f858b641a1bef19a3">Memory</enumvalue>
      <enumvalue file="classCorrade_1_1TestSuite_1_1Tester.html" anchor="a37efcdd6ea7a7ec4a8e4b304218308b4ae93f994f01c537c4e2f7d8528c3eb5e9">Count</enumvalue>
    </member>
    <member kind="typedef">
      <type>Corrade::Utility::Debug</type>
      <name>Debug</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a8ef15c8aec41c39215745963a3679199</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>Corrade::Utility::Warning</type>
      <name>Warning</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a48afcce4f459f33e54da2c3c45f6e54b</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>Corrade::Utility::Error</type>
      <name>Error</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a303019d6658462f4b8b365fde8d8c319</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; int &amp;, char ** &gt;</type>
      <name>arguments</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ad254f71ecc24a166536a99094d504090</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Tester</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a5e289b813e0071a0ef0bfe2b76f8013c</anchor>
      <arglist>(TesterConfiguration configuration=TesterConfiguration{})</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>aad65989bea7cf70d5579dd65ffa1ce9a</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addRepeatedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a06c40ddf62db02c1575d019b630d7f08</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t repeatCount)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a705b618d2aa631f2695d6c2adaf156ef</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, void(Derived::*setup)(), void(Derived::*teardown)())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addRepeatedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>aaa26e0cc2a3431ad8e5ee139079302c1</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t repeatCount, void(Derived::*setup)(), void(Derived::*teardown)())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addInstancedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>af63e77990b01434052ac1c55fd3bf05a</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t instanceCount)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addRepeatedInstancedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ac0fb10f0917037ac8dba8ca2e7d75ab6</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t repeatCount, std::size_t instanceCount)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addInstancedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>aa8e029e34201b65e661fa677d1f3234e</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t instanceCount, void(Derived::*setup)(), void(Derived::*teardown)())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addRepeatedInstancedTests</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ab56fc0bbd9681c4dddf158c04cc8f836</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; tests, std::size_t repeatCount, std::size_t instanceCount, void(Derived::*setup)(), void(Derived::*teardown)())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a424aeb9a3c2180464c155d982ae08389</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, BenchmarkType benchmarkType=BenchmarkType::Default)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>abd05bfd3a84e9bb0a280451df8e550e1</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, void(Derived::*setup)(), void(Derived::*teardown)(), BenchmarkType benchmarkType=BenchmarkType::Default)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addCustomBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>abc2586121a787554a70113a654a5b5c3</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, void(Derived::*benchmarkBegin)(), std::uint64_t(Derived::*benchmarkEnd)(), BenchmarkUnits benchmarkUnits)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addCustomBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a41a55bef9a39c9672ccfa434ad0f101b</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, void(Derived::*setup)(), void(Derived::*teardown)(), void(Derived::*benchmarkBegin)(), std::uint64_t(Derived::*benchmarkEnd)(), BenchmarkUnits benchmarkUnits)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addInstancedBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a65cbff7b6714412f59ec842678850d5a</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, std::size_t instanceCount, BenchmarkType benchmarkType=BenchmarkType::Default)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addInstancedBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a5e420c73f8909475bcc4b036a04bb57e</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, std::size_t instanceCount, void(Derived::*setup)(), void(Derived::*teardown)(), BenchmarkType benchmarkType=BenchmarkType::Default)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addCustomInstancedBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ad97e1564731958ec41099b7728cac53e</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, std::size_t instanceCount, void(Derived::*benchmarkBegin)(), std::uint64_t(Derived::*benchmarkEnd)(), BenchmarkUnits benchmarkUnits)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addCustomInstancedBenchmarks</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a75eeb5cd700bb72eccd27d7159656236</anchor>
      <arglist>(std::initializer_list&lt; void(Derived::*)()&gt; benchmarks, std::size_t batchCount, std::size_t instanceCount, void(Derived::*setup)(), void(Derived::*teardown)(), void(Derived::*benchmarkBegin)(), std::uint64_t(Derived::*benchmarkEnd)(), BenchmarkUnits benchmarkUnits)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>testCaseId</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ad31efe7dcc58d8b598f3a00d4bffe90b</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>testCaseInstanceId</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>aec893b37ef3cb922520cf18fe44d9588</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>testCaseRepeatId</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ae25a961212c79ee5652183b199357ea7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setTestCaseName</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a2137bdada941b228a60c3905f208c5ee</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setTestCaseName</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a76705bdce162388219938a6eea066671</anchor>
      <arglist>(std::string &amp;&amp;name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setTestCaseDescription</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a063f0a096ef06bdd093fb701ade5a794</anchor>
      <arglist>(const std::string &amp;description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setTestCaseDescription</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>adb4bbf7a4d372490ae939ef32deade9d</anchor>
      <arglist>(std::string &amp;&amp;description)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBenchmarkName</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>ad5f846fd0c8a95aaf04179e69a0ddb32</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBenchmarkName</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester.html</anchorfile>
      <anchor>a7f207ea485776874764b8f32f552d664</anchor>
      <arglist>(std::string &amp;&amp;name)</arglist>
    </member>
    <docanchor file="classCorrade_1_1TestSuite_1_1Tester">TestSuite-Tester-command-line</docanchor>
  </compound>
  <compound kind="class">
    <name>Corrade::TestSuite::Tester::TesterConfiguration</name>
    <filename>classCorrade_1_1TestSuite_1_1Tester_1_1TesterConfiguration.html</filename>
    <member kind="function">
      <type>const std::vector&lt; std::string &gt; &amp;</type>
      <name>skippedArgumentPrefixes</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester_1_1TesterConfiguration.html</anchorfile>
      <anchor>a0e7361302834c7c0e53ca2f5443bb86a</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>TesterConfiguration &amp;</type>
      <name>setSkippedArgumentPrefixes</name>
      <anchorfile>classCorrade_1_1TestSuite_1_1Tester_1_1TesterConfiguration.html</anchorfile>
      <anchor>a0a842a14181eb750bb9802c8a233455e</anchor>
      <arglist>(std::initializer_list&lt; std::string &gt; prefixes)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::AbstractHash</name>
    <filename>classCorrade_1_1Utility_1_1AbstractHash.html</filename>
    <templarg>digestSize</templarg>
    <member kind="enumvalue">
      <name>DigestSize</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a162fa31a26bf6c06ed8d23c417e2020ca1845bcbe98f41a624c92ed7e797a14f6</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>HashDigest&lt; digestSize &gt;</type>
      <name>Digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a44db12441a55b94fdead99c880fd0a4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DigestSize</name>
      <anchorfile>classCorrade_1_1Utility_1_1AbstractHash.html</anchorfile>
      <anchor>a162fa31a26bf6c06ed8d23c417e2020ca1845bcbe98f41a624c92ed7e797a14f6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::AndroidLogStreamBuffer</name>
    <filename>classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html</filename>
    <member kind="enumeration">
      <type></type>
      <name>LogPriority</name>
      <anchorfile>classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html</anchorfile>
      <anchor>a18ae87be1aa3d05f590a8ea2f26d538b</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538bad4a9fa383ab700c5bdd6f31cf7df0faf">Verbose</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538baa603905470e2a5b8c13e96b579ef0dba">Debug</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538ba4059b0251f66a18cb56f544728796875">Info</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538ba0eaadb4fcb48a0a0ed7bc9868be9fbaa">Warning</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538ba902b0d55fddef6f8d651fe1035b7d4bd">Error</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html" anchor="a18ae87be1aa3d05f590a8ea2f26d538ba882384ec38ce8d9582b57e70861730e4">Fatal</enumvalue>
    </member>
    <member kind="function">
      <type></type>
      <name>AndroidLogStreamBuffer</name>
      <anchorfile>classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html</anchorfile>
      <anchor>ab631a4d21e449fa7fc4ba337f6663b97</anchor>
      <arglist>(LogPriority priority, std::string tag)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type>int</type>
      <name>sync</name>
      <anchorfile>classCorrade_1_1Utility_1_1AndroidLogStreamBuffer.html</anchorfile>
      <anchor>ad18648077ccbb78dc0fc5c46b2166e7e</anchor>
      <arglist>() override</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Arguments</name>
    <filename>classCorrade_1_1Utility_1_1Arguments.html</filename>
    <member kind="function">
      <type></type>
      <name>Arguments</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a178ecfb6bbb9d3ca77aaffca138abed3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Arguments</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>adb7b197af6f46b29cdaab59032f6a78d</anchor>
      <arglist>(const std::string &amp;prefix)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addArgument</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>af1aa8eb7d9af745fcaa9042b8c915d96</anchor>
      <arglist>(std::string key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addNamedArgument</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>aa06be9626ed02314671e918867eed2b5</anchor>
      <arglist>(char shortKey, std::string key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addNamedArgument</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a2bf98b4f9617759235e19e5486cc8b0d</anchor>
      <arglist>(std::string key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addOption</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a5ef44d52ebeaa2eea906088ffe6e1537</anchor>
      <arglist>(char shortKey, std::string key, std::string defaultValue=std::string())</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addOption</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>abbe0aa87021d58e15fc2bf2fb34e065b</anchor>
      <arglist>(std::string key, std::string defaultValue=std::string())</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addBooleanOption</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a88e288320ad981adbd3410b3b5f7b355</anchor>
      <arglist>(char shortKey, std::string key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addBooleanOption</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>aa78f024b302dd98f0133113f64029f96</anchor>
      <arglist>(std::string key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>addSkippedPrefix</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a2b05c808d57135c37d6229fc250af38c</anchor>
      <arglist>(std::string prefix, std::string help={})</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setFromEnvironment</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a0bbc9455c3615ba42f6c1d4cb52f80d5</anchor>
      <arglist>(const std::string &amp;key, std::string environmentVariable)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setFromEnvironment</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a2da1065d616c7d5f6a32c8eb576c6f79</anchor>
      <arglist>(const std::string &amp;key)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setCommand</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a5d53692ba045cf61283d8c882dbb78c3</anchor>
      <arglist>(std::string name)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setHelp</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>ae2e7b65d81f67bf99efb7bc9b530417b</anchor>
      <arglist>(std::string help)</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setHelp</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a84a3e0606d26057ae6a71e5b645572b9</anchor>
      <arglist>(const std::string &amp;key, std::string help, std::string helpKey={})</arglist>
    </member>
    <member kind="function">
      <type>Arguments &amp;</type>
      <name>setHelpKey</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>abc9601633cf8db88800fa164f63957de</anchor>
      <arglist>(const std::string &amp;key, std::string helpKey)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>parse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>adb04863348b937593426df10f43d99d8</anchor>
      <arglist>(int argc, const char **argv)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>parse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a908e3c2f42fa3b659e15ff3f8e5ddd9d</anchor>
      <arglist>(int argc, char **argv)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>parse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a6f1911c2bc595f66f17f9f5675ca1db4</anchor>
      <arglist>(int argc, std::nullptr_t argv)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>tryParse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a5c11b3c6850c8c90602d9ffad0478f24</anchor>
      <arglist>(int argc, const char **argv)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>tryParse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a067f02f2a75c2f71b6e05c35345a073f</anchor>
      <arglist>(int argc, char **argv)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>tryParse</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a0c2bc66a2ca1a24d905019c48a644dd9</anchor>
      <arglist>(int argc, std::nullptr_t argv)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>usage</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a17f7c569dbdf0e741d9c0785aed07633</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>help</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>aaf01a1e96f793c92da2307dae4aa1f1d</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>value</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>ad3fa822ba398376739cfc98a80df89dd</anchor>
      <arglist>(const std::string &amp;key, ConfigurationValueFlags flags={}) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isSet</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>aa328bb8df10c76dbbee9ec97cdf1287b</anchor>
      <arglist>(const std::string &amp;key) const </arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::vector&lt; std::string &gt;</type>
      <name>environment</name>
      <anchorfile>classCorrade_1_1Utility_1_1Arguments.html</anchorfile>
      <anchor>a2310c3fb9f928461b621babd7a2c7e29</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Configuration</name>
    <filename>classCorrade_1_1Utility_1_1Configuration.html</filename>
    <base>Corrade::Utility::ConfigurationGroup</base>
    <member kind="enumeration">
      <type></type>
      <name>Flag</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a5d368fa6993f223b4c3fa25332921e86</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86afaebb611045e49cc0dfa08935d38be89">PreserveBom</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86a03cf6a68c2fac661330a1ad2cac38b90">ForceUnixEol</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86a6f417e6c5ec8c6620ad71177b1e4d3aa">ForceWindowsEol</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86aa8156810bfee2bd2b44765b9e91db3bd">Truncate</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86ae076500ec82f42a92c1291f3c2ebdb72">SkipComments</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Configuration.html" anchor="a5d368fa6993f223b4c3fa25332921e86a131fb182a881796e7606ed6da27f1197">ReadOnly</enumvalue>
    </member>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; Flag &gt;</type>
      <name>Flags</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a0b3f492e864ee97228d5170d38a31933</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>abe8c1af9685dc46bce30ee46203f895c</anchor>
      <arglist>(Flags flags=Flags())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a5aa6826a44ae881fdd9a3b7ff4b57535</anchor>
      <arglist>(const std::string &amp;filename, Flags flags=Flags())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>aa9808d44a604e4a455975e3ebfd6bf37</anchor>
      <arglist>(std::istream &amp;in, Flags flags=Flags())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a7ac737721e13c9303696985e2a8672e1</anchor>
      <arglist>(const Configuration &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a1258e56c643add518646e32edaf99892</anchor>
      <arglist>(Configuration &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~Configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>ae48021eb27720034fdebaa9dd1adad27</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Configuration &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>ab54d47655023263934e52520ba00ba4a</anchor>
      <arglist>(const Configuration &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Configuration &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a755a0d60b854f2a78e5f6eccd9fe452b</anchor>
      <arglist>(Configuration &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>filename</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a6e79eb80aa4212e263d04264a62352f5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setFilename</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>aa8745aca008e75311acbb3778ba81fd0</anchor>
      <arglist>(std::string filename)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isValid</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a8cd375ba809a635da96186a150d38f5d</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>save</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>a4e6f38caab000dd04321001f4445d1ff</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>save</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>abc47d18cdd33e5c928f2aa33dd6b09fd</anchor>
      <arglist>(std::ostream &amp;out)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>save</name>
      <anchorfile>classCorrade_1_1Utility_1_1Configuration.html</anchorfile>
      <anchor>ae6d938117a8afa18976fc9a481220f2e</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::ConfigurationGroup</name>
    <filename>classCorrade_1_1Utility_1_1ConfigurationGroup.html</filename>
    <member kind="function">
      <type></type>
      <name>ConfigurationGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a500d96ba04e6c01b95d9121cf4934f18</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ConfigurationGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>ab41d1f58bfcf9d6b7cb5727f87429e1e</anchor>
      <arglist>(const ConfigurationGroup &amp;other)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ConfigurationGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1ae4cecc4e8cf32c7e4d3aaa286bdc59</anchor>
      <arglist>(ConfigurationGroup &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a291f0b9770236cc43b9ccbc17ea2b6f3</anchor>
      <arglist>(const ConfigurationGroup &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1172fdd7703f23820d7c92057e3b86e4</anchor>
      <arglist>(ConfigurationGroup &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>Configuration *</type>
      <name>configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a6ac83e7385b39cd9a1c08aeb9a7201d8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const Configuration *</type>
      <name>configuration</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a29d8c467b34339a4f1730e78dabf8817</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isEmpty</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a722da7c97ce2af7c6d162808ba9e6119</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clear</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a527a6fb19869f89e99ae052d1d26a5e5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasGroups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1af34d0d38a2142baf50feb8f228ba61</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>groupCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa088c7831238e4d5b62231b3a8eb95af</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>ae057d16fe0c0303f3dca92ff39d3a87a</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>groupCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a12ea9fcb5241c1c0f70375f81398961e</anchor>
      <arglist>(const std::string &amp;name) const </arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup *</type>
      <name>group</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a2bc02705571e6eb3fee1077c9c61a573</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>const ConfigurationGroup *</type>
      <name>group</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>af92bfe4bc981e37844072ee6ae80f515</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; ConfigurationGroup * &gt;</type>
      <name>groups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa5c8f92f48ac44d7d3d084622e6df6cc</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; const ConfigurationGroup * &gt;</type>
      <name>groups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a4286fcfbe9ade8a026a73e49ea4e93b3</anchor>
      <arglist>(const std::string &amp;name) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a283661713aa073739663b46f5c468e1c</anchor>
      <arglist>(const std::string &amp;name, ConfigurationGroup *group)</arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup *</type>
      <name>addGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a0d97d4fd435ba78a1c8d96aca8019b75</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a6eeed46dd76a629fd2693732c91d0040</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>afe24d64f8c78513293e57e38c7d0727c</anchor>
      <arglist>(ConfigurationGroup *group)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeAllGroups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a84000627f3d7e0e07d0a6ffa09c2c636</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasValues</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a88f8decfd75d37a95bfe4c4414f1cf74</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>valueCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a8e0e8fe4170865302e35ecd5234eb13f</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a5718b8bf27da8ff01c90b555cb8791fb</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>valueCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>ae2553337e66dd134c40a39dd8e9fd5f1</anchor>
      <arglist>(const std::string &amp;key) const </arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>value</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a76ea58bcdcfc5bc156327d45ed921ef3</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags()) const </arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>value</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a2acad127e5628c963f9fe4ba6786c1c1</anchor>
      <arglist>(const std::string &amp;key, ConfigurationValueFlags flags) const </arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; T &gt;</type>
      <name>values</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a7f4ec39138554c01739c7a3b10bc4462</anchor>
      <arglist>(const std::string &amp;key, ConfigurationValueFlags flags=ConfigurationValueFlags()) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aadca4a5b40bcd66d0837193af2c841e2</anchor>
      <arglist>(const std::string &amp;key, std::string value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a8d881c499196d832b7dd3581df25ab55</anchor>
      <arglist>(const std::string &amp;key, const char *value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1be7095f3d51a0d6fe324545c62d569e</anchor>
      <arglist>(const std::string &amp;key, std::string value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a025de56172c2b6ff9640053b6a72eb00</anchor>
      <arglist>(const std::string &amp;key, const char *value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa2988c7a715adb37b3d8938a08b9e006</anchor>
      <arglist>(const std::string &amp;key, const T &amp;value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>abf56c0ee12977dd024ede9cfcd1fda41</anchor>
      <arglist>(const std::string &amp;key, const T &amp;value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>abf66026df64ae2561040aaa75fdfbd31</anchor>
      <arglist>(std::string key, std::string value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a6ba87151d0821e99d9e5038586ffed71</anchor>
      <arglist>(std::string key, const char *value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aee6029a57056ff1e3a4400d8da424c48</anchor>
      <arglist>(std::string key, const T &amp;value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1b7d7d5fbd18eaf10c3aafe4871509ad</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeAllValues</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1654bb0a8323a813f380004a6fb57759</anchor>
      <arglist>(const std::string &amp;key)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasGroups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1af34d0d38a2142baf50feb8f228ba61</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>groupCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa088c7831238e4d5b62231b3a8eb95af</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>ae057d16fe0c0303f3dca92ff39d3a87a</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>groupCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a12ea9fcb5241c1c0f70375f81398961e</anchor>
      <arglist>(const std::string &amp;name) const </arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup *</type>
      <name>group</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a2bc02705571e6eb3fee1077c9c61a573</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>const ConfigurationGroup *</type>
      <name>group</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>af92bfe4bc981e37844072ee6ae80f515</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; ConfigurationGroup * &gt;</type>
      <name>groups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa5c8f92f48ac44d7d3d084622e6df6cc</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; const ConfigurationGroup * &gt;</type>
      <name>groups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a4286fcfbe9ade8a026a73e49ea4e93b3</anchor>
      <arglist>(const std::string &amp;name) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a283661713aa073739663b46f5c468e1c</anchor>
      <arglist>(const std::string &amp;name, ConfigurationGroup *group)</arglist>
    </member>
    <member kind="function">
      <type>ConfigurationGroup *</type>
      <name>addGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a0d97d4fd435ba78a1c8d96aca8019b75</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a6eeed46dd76a629fd2693732c91d0040</anchor>
      <arglist>(const std::string &amp;name, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>afe24d64f8c78513293e57e38c7d0727c</anchor>
      <arglist>(ConfigurationGroup *group)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeAllGroups</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a84000627f3d7e0e07d0a6ffa09c2c636</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasValues</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a88f8decfd75d37a95bfe4c4414f1cf74</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>valueCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a8e0e8fe4170865302e35ecd5234eb13f</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>hasValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a5718b8bf27da8ff01c90b555cb8791fb</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0) const </arglist>
    </member>
    <member kind="function">
      <type>unsigned int</type>
      <name>valueCount</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>ae2553337e66dd134c40a39dd8e9fd5f1</anchor>
      <arglist>(const std::string &amp;key) const </arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>value</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a76ea58bcdcfc5bc156327d45ed921ef3</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags()) const </arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>value</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a2acad127e5628c963f9fe4ba6786c1c1</anchor>
      <arglist>(const std::string &amp;key, ConfigurationValueFlags flags) const </arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; T &gt;</type>
      <name>values</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a7f4ec39138554c01739c7a3b10bc4462</anchor>
      <arglist>(const std::string &amp;key, ConfigurationValueFlags flags=ConfigurationValueFlags()) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aadca4a5b40bcd66d0837193af2c841e2</anchor>
      <arglist>(const std::string &amp;key, std::string value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a8d881c499196d832b7dd3581df25ab55</anchor>
      <arglist>(const std::string &amp;key, const char *value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1be7095f3d51a0d6fe324545c62d569e</anchor>
      <arglist>(const std::string &amp;key, std::string value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a025de56172c2b6ff9640053b6a72eb00</anchor>
      <arglist>(const std::string &amp;key, const char *value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aa2988c7a715adb37b3d8938a08b9e006</anchor>
      <arglist>(const std::string &amp;key, const T &amp;value, unsigned int index=0, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>setValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>abf56c0ee12977dd024ede9cfcd1fda41</anchor>
      <arglist>(const std::string &amp;key, const T &amp;value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>abf66026df64ae2561040aaa75fdfbd31</anchor>
      <arglist>(std::string key, std::string value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a6ba87151d0821e99d9e5038586ffed71</anchor>
      <arglist>(std::string key, const char *value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>aee6029a57056ff1e3a4400d8da424c48</anchor>
      <arglist>(std::string key, const T &amp;value, ConfigurationValueFlags flags=ConfigurationValueFlags())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>removeValue</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1b7d7d5fbd18eaf10c3aafe4871509ad</anchor>
      <arglist>(const std::string &amp;key, unsigned int index=0)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeAllValues</name>
      <anchorfile>classCorrade_1_1Utility_1_1ConfigurationGroup.html</anchorfile>
      <anchor>a1654bb0a8323a813f380004a6fb57759</anchor>
      <arglist>(const std::string &amp;key)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue.html</filename>
    <templarg></templarg>
    <member kind="function" static="yes">
      <type>static std::string</type>
      <name>toString</name>
      <anchorfile>structCorrade_1_1Utility_1_1ConfigurationValue.html</anchorfile>
      <anchor>ab639d070423f77173baa8ada58975ffa</anchor>
      <arglist>(const T &amp;value, ConfigurationValueFlags flags)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static T</type>
      <name>fromString</name>
      <anchorfile>structCorrade_1_1Utility_1_1ConfigurationValue.html</anchorfile>
      <anchor>a5e3bd19c82a34fb97bfddc94404f932a</anchor>
      <arglist>(const std::string &amp;stringValue, ConfigurationValueFlags flags)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; bool &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01bool_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; char32_t &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01char32__t_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; double &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01double_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; float &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01float_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; int &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01int_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; long &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01long_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; long double &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01long_01double_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; long long &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01long_01long_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; short &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01short_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; std::string &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01std_1_1string_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; unsigned int &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01unsigned_01int_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; unsigned long &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01unsigned_01long_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; unsigned long long &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01unsigned_01long_01long_01_4.html</filename>
  </compound>
  <compound kind="struct">
    <name>Corrade::Utility::ConfigurationValue&lt; unsigned short &gt;</name>
    <filename>structCorrade_1_1Utility_1_1ConfigurationValue_3_01unsigned_01short_01_4.html</filename>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Debug</name>
    <filename>classCorrade_1_1Utility_1_1Debug.html</filename>
    <member kind="enumeration">
      <type></type>
      <name>Flag</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>aa346e932b5141006c49777ddacfaf600</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="aa346e932b5141006c49777ddacfaf600af29e1bcd00663e0b5d5a74d31baa9e96">NoNewlineAtTheEnd</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="aa346e932b5141006c49777ddacfaf600ada953bf4ea2f8e90228cd77eb880389f">DisableColors</enumvalue>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>Color</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a40f3e8edbd24f69725130f20202bbc2d</anchor>
      <arglist></arglist>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2dae90dfb84e30edf611e326eeb04d680de">Black</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2daee38e4d5dd68c4e440825018d549cb47">Red</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2dad382816a3cbeed082c9e216e7392eed1">Green</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2da51e6cd92b6c45f9affdc158ecca2b8b8">Yellow</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2da9594eec95be70e7b1710f730fdda33d9">Blue</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2dab91cc2c1416fcca942b61c7ac5b1a9ac">Magenta</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2da023c239d2f2538f140a20e72c7b73f20">Cyan</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2da25a81701fbfa4a1efdf660a950c1d006">White</enumvalue>
      <enumvalue file="classCorrade_1_1Utility_1_1Debug.html" anchor="a40f3e8edbd24f69725130f20202bbc2da7a1920d61156abc05a60135aefe8bc67">Default</enumvalue>
    </member>
    <member kind="typedef">
      <type>void(*</type>
      <name>Modifier</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>aede8a384033583c6b407e28134fd7c8a</anchor>
      <arglist>)(Debug &amp;)</arglist>
    </member>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; Flag &gt;</type>
      <name>Flags</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>aedc4d6996ab47a86c346c1667794729b</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Debug</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a069a145924bd86bfc3f4c6af07508452</anchor>
      <arglist>(Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Debug</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a094bd4c780a495f91dbe19776127f4f8</anchor>
      <arglist>(std::ostream *output, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Debug</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a5eb811dee5401f2382acb8476f6d837a</anchor>
      <arglist>(const Debug &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Debug</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a11fa7c20ee652f1da6fe75d475ab0e48</anchor>
      <arglist>(Debug &amp;&amp;)=default</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~Debug</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a4e30b5f23e9d3f56bb54942ef11288e8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a621047711b250b30e87cf7143e429f0c</anchor>
      <arglist>(const Debug &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a02b17ddadb7259779431bafdb6dc84a2</anchor>
      <arglist>(Debug &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ab2f5931b3aebb43f4e8216d7188ba5a8</anchor>
      <arglist>(const std::string &amp;value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a713f25af5a460b8f3cf6e9b7203e8ebe</anchor>
      <arglist>(const char *value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ab2b2895da70cbda6f19f62a0e92ad138</anchor>
      <arglist>(const void *value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a4e34d1c961f28757b6cfe6ae214950e0</anchor>
      <arglist>(bool value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a213a10e849be7d791c38f28fea654d63</anchor>
      <arglist>(int value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>af494a2b8261c74262966410ba0cd6677</anchor>
      <arglist>(long value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a992bbaf961e11709a6f47c42872621e4</anchor>
      <arglist>(long long value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ace984244c193e9c40a86f17df3e6be2b</anchor>
      <arglist>(unsigned value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a5068548b7e32460f24b3e77cbc361ba0</anchor>
      <arglist>(unsigned long value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ab5e5e420b2ffbe746fe5238740a89a47</anchor>
      <arglist>(unsigned long long value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a64747bfa8b6b530d0d079d296d23eb33</anchor>
      <arglist>(float value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a3bbb7769e9e3dd7a6cf59e22221c6d71</anchor>
      <arglist>(double value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a27a4f78e38dcc3bdc8371da13c554038</anchor>
      <arglist>(long double value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a580d97e0003cd26285ce8201687473ba</anchor>
      <arglist>(char32_t value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ac29e10def861342c58687f4aa679b285</anchor>
      <arglist>(const char32_t *value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ac5f5e43d351399cc0dc1e471a753a300</anchor>
      <arglist>(Modifier f)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Debug</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a81b88bd4e3b2f1f3182246cd5e3a9db4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Debug</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ac5a767110ca7d82d723c7886c2317ca9</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>nospace</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>aca4f49eaa3129ae56f8223a499377f11</anchor>
      <arglist>(Debug &amp;debug)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>newline</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a8a96ee3513ec028eddbc520356293213</anchor>
      <arglist>(Debug &amp;debug)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Modifier</type>
      <name>color</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a4e59d103a5f719596fd14ea12677f928</anchor>
      <arglist>(Color color)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Modifier</type>
      <name>boldColor</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a522e0f0e38056d5592f4ae8b757ddfa6</anchor>
      <arglist>(Color color)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>resetColor</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>adf358e663180cb94e812f73dedc4f86c</anchor>
      <arglist>(Debug &amp;debug)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>setOutput</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>aa383926097f8ed1a604e62037859eac5</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>isTty</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a35394a39a5df4322a36a598557c07db1</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>isTty</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a32458743e3d3731036378cf8577c7934</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ae2cee4795e84d13d29d1d9e1c805ecd2</anchor>
      <arglist>(Debug &amp;debug, const T &amp;value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>ac103e9217a8710bd03ee4783d3d6ed4b</anchor>
      <arglist>(Debug &amp;debug, const Iterable &amp;value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a9f9d27d6f040e6e490f2d73c5a0b8b56</anchor>
      <arglist>(Debug &amp;debug, const std::tuple&lt; Args... &gt; &amp;value)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Debug.html</anchorfile>
      <anchor>a76d39b67ac31b0abf23b5c5d4bec9f15</anchor>
      <arglist>(Debug &amp;debug, const std::pair&lt; T, U &gt; &amp;value)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Endianness</name>
    <filename>classCorrade_1_1Utility_1_1Endianness.html</filename>
    <member kind="function" static="yes">
      <type>static T</type>
      <name>swap</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>a37b31ac2f71e436d1d090b7d9a27973e</anchor>
      <arglist>(T value)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static constexpr bool</type>
      <name>isBigEndian</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>a9d7a7670b74586ed27761f5d7c330ccc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static T</type>
      <name>bigEndian</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>ab9447075a4f52305ecc520228a3520ff</anchor>
      <arglist>(T value)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>bigEndianInPlace</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>aa8d5cfe890bb35f72c2a8836b2d44073</anchor>
      <arglist>(T &amp;...values)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static T</type>
      <name>littleEndian</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>a14ff651446505859b617f2daaf94e0f0</anchor>
      <arglist>(T number)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>littleEndianInPlace</name>
      <anchorfile>classCorrade_1_1Utility_1_1Endianness.html</anchorfile>
      <anchor>a4371cce40d6689408136b8012df9c7ab</anchor>
      <arglist>(T &amp;...values)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Error</name>
    <filename>classCorrade_1_1Utility_1_1Error.html</filename>
    <base>Corrade::Utility::Debug</base>
    <member kind="function">
      <type></type>
      <name>Error</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a3fbe9b0f2557ce83055671246dcf7e7e</anchor>
      <arglist>(Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Error</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a213951517715c247dac75d596c2d13f8</anchor>
      <arglist>(std::ostream *output, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Error</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>aabe83634a34265d09ec611d1d9e21914</anchor>
      <arglist>(const Error &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Error</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a792f4425e6734e77e99ae29be4fb9572</anchor>
      <arglist>(Error &amp;&amp;)=default</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~Error</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>aea88b9362728fd3306d67b80b65994a7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Error &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a1283baaa3477cbc49d48bfad02d57e5a</anchor>
      <arglist>(const Error &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Error &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a23c200ee5c7044cd25ce4d32a23d5f7a</anchor>
      <arglist>(Error &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Error</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>ade0da903ca3f3270b3bc33f757e55569</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Error</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>af81b3202551779997eac890745fc31df</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>setOutput</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a30ad23439c87546e85b787a30c6f89ac</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>isTty</name>
      <anchorfile>classCorrade_1_1Utility_1_1Error.html</anchorfile>
      <anchor>a8b9c5f47adfa98178a8830959aa14da1</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Fatal</name>
    <filename>classCorrade_1_1Utility_1_1Fatal.html</filename>
    <base>Corrade::Utility::Error</base>
    <member kind="function">
      <type></type>
      <name>Fatal</name>
      <anchorfile>classCorrade_1_1Utility_1_1Fatal.html</anchorfile>
      <anchor>ace9441b319771c44b9407736733d9489</anchor>
      <arglist>(int exitCode=1, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Fatal</name>
      <anchorfile>classCorrade_1_1Utility_1_1Fatal.html</anchorfile>
      <anchor>a2b8532632da13ea29fe0aac2f8cc30d5</anchor>
      <arglist>(Flags flags)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Fatal</name>
      <anchorfile>classCorrade_1_1Utility_1_1Fatal.html</anchorfile>
      <anchor>acd4ec7626fa35d43d0d7453d34fe7874</anchor>
      <arglist>(std::ostream *output, int exitCode=1, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Fatal</name>
      <anchorfile>classCorrade_1_1Utility_1_1Fatal.html</anchorfile>
      <anchor>a5960d6dab2e70b5c8244a2d263ee5ab0</anchor>
      <arglist>(std::ostream *output, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type>CORRADE_NORETURN</type>
      <name>~Fatal</name>
      <anchorfile>classCorrade_1_1Utility_1_1Fatal.html</anchorfile>
      <anchor>a7a65b11e97ce23312a7c62e9d52cd847</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::HashDigest</name>
    <filename>classCorrade_1_1Utility_1_1HashDigest.html</filename>
    <templarg>size</templarg>
    <member kind="function">
      <type>constexpr</type>
      <name>HashDigest</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>abb77f485b60ebc66137e58945ed1d27e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>HashDigest</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>ab61dda1fd9e0dd7d6248819178674b0e</anchor>
      <arglist>(T firstValue, U...nextValues)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>a475792d3ca0e10e5f9f3ba4a044e8246</anchor>
      <arglist>(const HashDigest&lt; size &gt; &amp;other) const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator!=</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>aa6452712cdb0dd8de51f6ab92189d83d</anchor>
      <arglist>(const HashDigest&lt; size &gt; &amp;other) const </arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>hexString</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>af7ecaaf3dbe9d791dcdad772e2f25320</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>constexpr const char *</type>
      <name>byteArray</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>ac502db42aee144066a7aad31bf3fc8c2</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" static="yes">
      <type>static HashDigest&lt; size &gt;</type>
      <name>fromHexString</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>a195da27f46772170af3ef3b647c52361</anchor>
      <arglist>(std::string digest)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static const HashDigest&lt; size &gt; &amp;</type>
      <name>fromByteArray</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>add330139edc8634f4647832bff793b0c</anchor>
      <arglist>(const char *digest)</arglist>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1HashDigest.html</anchorfile>
      <anchor>a39667d2dab7d417582ae04c76fd8af78</anchor>
      <arglist>(Debug &amp;debug, const HashDigest&lt; size &gt; &amp;value)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::MurmurHash2</name>
    <filename>classCorrade_1_1Utility_1_1MurmurHash2.html</filename>
    <base>AbstractHash&lt; sizeof(std::size_t)&gt;</base>
    <member kind="function">
      <type>constexpr</type>
      <name>MurmurHash2</name>
      <anchorfile>classCorrade_1_1Utility_1_1MurmurHash2.html</anchorfile>
      <anchor>a7b512789e9557ebc5d04429af1b29c85</anchor>
      <arglist>(std::size_t seed=0)</arglist>
    </member>
    <member kind="function">
      <type>Digest</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1Utility_1_1MurmurHash2.html</anchorfile>
      <anchor>adc237af6c0003c9c0ce949a3399d8895</anchor>
      <arglist>(const std::string &amp;data) const </arglist>
    </member>
    <member kind="function">
      <type>Digest</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1Utility_1_1MurmurHash2.html</anchorfile>
      <anchor>a60e232949ab5c7b840a52d4d77305568</anchor>
      <arglist>(const char(&amp;data)[size]) const </arglist>
    </member>
    <member kind="function">
      <type>Digest</type>
      <name>operator()</name>
      <anchorfile>classCorrade_1_1Utility_1_1MurmurHash2.html</anchorfile>
      <anchor>a046db82ced985bb398ac9a45cc212479</anchor>
      <arglist>(const char *data, std::size_t size) const </arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Digest</type>
      <name>digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1MurmurHash2.html</anchorfile>
      <anchor>abca6f6111dc7e38921fc15ba22ff885b</anchor>
      <arglist>(const std::string &amp;data)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Resource</name>
    <filename>classCorrade_1_1Utility_1_1Resource.html</filename>
    <member kind="function">
      <type></type>
      <name>Resource</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>afbf57511408670c4e52d987fae39d086</anchor>
      <arglist>(const std::string &amp;group)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>list</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>a07b62771bf4fd6d65db057f3a19a6cab</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Containers::ArrayView&lt; const char &gt;</type>
      <name>getRaw</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>aeab3bc7f28cd9d8da576243e6a4c5f7e</anchor>
      <arglist>(const std::string &amp;filename) const </arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>get</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>a089287b6126538329819449a3eba3158</anchor>
      <arglist>(const std::string &amp;filename) const </arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::string</type>
      <name>compile</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>ae27c0dcd044bf1ed9a4778a12cf73735</anchor>
      <arglist>(const std::string &amp;name, const std::string &amp;group, const std::vector&lt; std::pair&lt; std::string, std::string &gt;&gt; &amp;files)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::string</type>
      <name>compileFrom</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>a19b48421cde009ddd1fa04a2e1b4ff0e</anchor>
      <arglist>(const std::string &amp;name, const std::string &amp;configurationFile)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>overrideGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>a24f3eacc460abf21d6a3a439bba988cb</anchor>
      <arglist>(const std::string &amp;group, const std::string &amp;configurationFile)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>hasGroup</name>
      <anchorfile>classCorrade_1_1Utility_1_1Resource.html</anchorfile>
      <anchor>a4e1b9ed0b2f0f744035ec5280217befe</anchor>
      <arglist>(const std::string &amp;group)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Sha1</name>
    <filename>classCorrade_1_1Utility_1_1Sha1.html</filename>
    <base>AbstractHash&lt; 20 &gt;</base>
    <member kind="function">
      <type>Sha1 &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>classCorrade_1_1Utility_1_1Sha1.html</anchorfile>
      <anchor>aacf2b0b3f154574aa69951e8e008584c</anchor>
      <arglist>(const std::string &amp;data)</arglist>
    </member>
    <member kind="function">
      <type>Digest</type>
      <name>digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1Sha1.html</anchorfile>
      <anchor>a9f933c6680b1dcf1bf185429c8b571ec</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Digest</type>
      <name>digest</name>
      <anchorfile>classCorrade_1_1Utility_1_1Sha1.html</anchorfile>
      <anchor>a58ae2de7f39775cb7fb3594b80e98be5</anchor>
      <arglist>(const std::string &amp;data)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>Corrade::Utility::Warning</name>
    <filename>classCorrade_1_1Utility_1_1Warning.html</filename>
    <base>Corrade::Utility::Debug</base>
    <member kind="function">
      <type></type>
      <name>Warning</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a6afe64c565a1ebae35228b840e63c428</anchor>
      <arglist>(Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Warning</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a92f3ddd2c7e6caed6b6967d0f72c001b</anchor>
      <arglist>(std::ostream *output, Flags flags={})</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Warning</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a742946b1583b9e147a1b502c647fc1ec</anchor>
      <arglist>(const Warning &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Warning</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>accd8b00e770a072a122c4ffbc7b2373c</anchor>
      <arglist>(Warning &amp;&amp;)=default</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~Warning</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a66833257bc5b0d126add892b19091c56</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>Warning &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a10bd7464c247b4df94adb4ec6ab26c6e</anchor>
      <arglist>(const Warning &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>Warning &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>adf0d29c428d7f4a094debd445a5a1516</anchor>
      <arglist>(Warning &amp;&amp;)=delete</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Warning</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>ad353ad204326b36394ba4dc8ae71831e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static Warning</type>
      <name>noNewlineAtTheEnd</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a5fe9d9063945bf3244c05f17df19cb50</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>setOutput</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a6d7c48994b6b03bf7703a77ffe4ec632</anchor>
      <arglist>(std::ostream *output)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>isTty</name>
      <anchorfile>classCorrade_1_1Utility_1_1Warning.html</anchorfile>
      <anchor>a5b0ee54b7afbcd80816575963bf01702</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>EnumSet&lt; InternalFlag &gt;</name>
    <filename>classCorrade_1_1Containers_1_1EnumSet.html</filename>
    <member kind="typedef">
      <type>InternalFlag</type>
      <name>Type</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>af79d534425b3439095c579944858bb4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>std::underlying_type&lt; InternalFlag &gt;::type</type>
      <name>UnderlyingType</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ae98013f13d62ca0bac69d632905846c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FullValue</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>aee87dfd8eec50c26f5f9703feea66abda9e456595199d05472bf7927d329639b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ac3820645815d705507170a3e507087b5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>aff6e8c4b80a660f381b31a2ed5106695</anchor>
      <arglist>(InternalFlagvalue)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>EnumSet</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>adb0d3bd1925b399503dc745e51c245a6</anchor>
      <arglist>(NoInitT)</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator==</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a46221489ad6521358ffcf7845f9d3462</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator!=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a7cb7f4bd84490b718800c3ba2f1df17e</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator&gt;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a3885d6aa88442661407ee1a9ecb86ec3</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>operator&lt;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a42906634012570f68eb709b6c5774053</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt;</type>
      <name>operator|</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>ac6c4a74c4c567c7b5b6174c50203113a</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; &amp;</type>
      <name>operator|=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a0570b5800f24317a1b52b27d878b9bfa</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt;</type>
      <name>operator&amp;</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a61cf465a2d9cb28bcb3a139f6db20f98</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; &amp;</type>
      <name>operator&amp;=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a882bec7925638c92d8491ce07108f398</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt;</type>
      <name>operator^</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a6cf9b501b4fb1c5c83b900defc7d2ebf</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other) const</arglist>
    </member>
    <member kind="function">
      <type>EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; &amp;</type>
      <name>operator^=</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>afc5caf6f9260aedc01b5d532e4143e36</anchor>
      <arglist>(EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt; other)</arglist>
    </member>
    <member kind="function">
      <type>constexpr EnumSet&lt; InternalFlag, typename std::underlying_type&lt; InternalFlag &gt;::type(~0) &gt;</type>
      <name>operator~</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a37f0da0636b0e7fbca8a4f3ed7c59c9c</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator bool</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>a999c11f444c7589b574ea80e9087c1d9</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr</type>
      <name>operator UnderlyingType</name>
      <anchorfile>classCorrade_1_1Containers_1_1EnumSet.html</anchorfile>
      <anchor>abb5a6545e21fd33af55c6d20a0e3277f</anchor>
      <arglist>() const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>LinkedList&lt; Item &gt;</name>
    <filename>classCorrade_1_1Containers_1_1LinkedList.html</filename>
    <member kind="function">
      <type>constexpr</type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>aef8b2c6fac22b76d6510b030de0bd7ff</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ae30e2ad2d45ed813d5b6517da99c2c45</anchor>
      <arglist>(const LinkedList&lt; Item &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a20a700a56461d5b8a6892ca147384609</anchor>
      <arglist>(LinkedList&lt; Item &gt; &amp;&amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~LinkedList</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ab9bdae1a80b28b53f4b70edd139c2741</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>LinkedList&lt; Item &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a93fe27abbdca0a2bdfffbb0c1c9e52b2</anchor>
      <arglist>(const LinkedList&lt; Item &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>LinkedList&lt; Item &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a77a0c2633a85000db72a6acf84cfde45</anchor>
      <arglist>(LinkedList&lt; Item &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>Item *</type>
      <name>first</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a9d49452ca29d53f6313285e8426ec13a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr const Item *</type>
      <name>first</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a7bd1e169057986db30b9b54da8bae08c</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>Item *</type>
      <name>last</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>abdb5e04bfec6bde39d8a20dd8d4d4674</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>constexpr const Item *</type>
      <name>last</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a62c2d194213d6af1702da006769f2f8a</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>isEmpty</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a44ca290841e84a0df958a797eea3f17d</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>insert</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>ab85682283d7ef74f20a2c59a3153f9a1</anchor>
      <arglist>(Item *item, Item *before=nullptr)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>cut</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a43d45cf92d3f88f450047b773e5fe157</anchor>
      <arglist>(Item *item)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>move</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a39ec134725e0b19f1d9bdfcd5e807973</anchor>
      <arglist>(Item *item, Item *before)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>erase</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a5d42a873dd83fbc0cc0084209536f9be</anchor>
      <arglist>(Item *item)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clear</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedList.html</anchorfile>
      <anchor>a83550122caae33c90d11d45d0b747184</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>LinkedListItem&lt; Item &gt;</name>
    <filename>classCorrade_1_1Containers_1_1LinkedListItem.html</filename>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a61ce16db8160e1d80e881d5761f4752b</anchor>
      <arglist>() noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>abe1d3f91dd60c7f44e177f6b4c06c016</anchor>
      <arglist>(const LinkedListItem&lt; Item, LinkedList&lt; Item &gt; &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a5a9b570f0fe78f27598febccdae1ddde</anchor>
      <arglist>(LinkedListItem&lt; Item, LinkedList&lt; Item &gt; &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function">
      <type>LinkedListItem &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>acd27fdb8bced1051b1a0ea2c8f866f18</anchor>
      <arglist>(const LinkedListItem&lt; Item, LinkedList&lt; Item &gt; &gt; &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>LinkedListItem&lt; Item, LinkedList&lt; Item &gt; &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>af9f41d1506c9ef6ef4e4f0bb8f3882ef</anchor>
      <arglist>(LinkedListItem&lt; Item, LinkedList&lt; Item &gt; &gt; &amp;&amp;other)</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual</type>
      <name>~LinkedListItem</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a0863b9d8b2ea61e35ff0aac3780870cb</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function">
      <type>LinkedList&lt; Item &gt; *</type>
      <name>list</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>ab607f167b90d2b00e42b238240afb36d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const LinkedList&lt; Item &gt; *</type>
      <name>list</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>aeb0b84fe7899d4229b1f229d095d6ccd</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>Item *</type>
      <name>previous</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a600d8bd1ebea7689159d9c37e1302fc4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const Item *</type>
      <name>previous</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a3dff4a6d49d907850860f9077f7bd5ce</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>Item *</type>
      <name>next</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>ac878e7e23048b57e2e577f73ff2d4870</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const Item *</type>
      <name>next</name>
      <anchorfile>classCorrade_1_1Containers_1_1LinkedListItem.html</anchorfile>
      <anchor>a31c7f69f017a70ddcdf6ae8a9bbce24a</anchor>
      <arglist>() const</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade</name>
    <filename>namespaceCorrade.html</filename>
    <namespace>Corrade::Containers</namespace>
    <namespace>Corrade::Interconnect</namespace>
    <namespace>Corrade::PluginManager</namespace>
    <namespace>Corrade::TestSuite</namespace>
    <namespace>Corrade::Utility</namespace>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Containers</name>
    <filename>namespaceCorrade_1_1Containers.html</filename>
    <class kind="class">Corrade::Containers::Array</class>
    <class kind="class">Corrade::Containers::ArrayView</class>
    <class kind="class">Corrade::Containers::ArrayView&lt; const void &gt;</class>
    <class kind="struct">Corrade::Containers::DefaultInitT</class>
    <class kind="struct">Corrade::Containers::DirectInitT</class>
    <class kind="class">Corrade::Containers::EnumSet</class>
    <class kind="struct">Corrade::Containers::InPlaceInitT</class>
    <class kind="class">Corrade::Containers::LinkedList</class>
    <class kind="class">Corrade::Containers::LinkedListItem</class>
    <class kind="struct">Corrade::Containers::NoInitT</class>
    <class kind="class">Corrade::Containers::StaticArray</class>
    <class kind="class">Corrade::Containers::StaticArrayView</class>
    <class kind="struct">Corrade::Containers::ValueInitT</class>
    <member kind="typedef">
      <type>ArrayView&lt; T &gt;</type>
      <name>ArrayReference</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aba27f25d31d10b5ffa38960b49e05ed8</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a57f8d4e0a25951f539461539c90d4e25</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a4258433529f847d3e6ed10442f6b9b8b</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ad5f0615634b2553b0cb304aa58da3347</anchor>
      <arglist>(Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; const U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a9a1ca1efe52f9930f1b31b9feab5a145</anchor>
      <arglist>(const Array&lt; T, D &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>af8a4e6b17dca34a5eb82b6a8da8c398b</anchor>
      <arglist>(const Array&lt; T &gt; &amp;view)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a32ef8daa386aee742e5f39fc5b1516fd</anchor>
      <arglist>(T *data, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ac8edb9cd0a74f5a0da2ef9c6d03f9ef2</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ac450a49515ff20058552c732542b1b13</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>ArrayView&lt; U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a8e6435208f2cda38505084a930a3e9ec</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a00abca03029b326133e9e7c908508af4</anchor>
      <arglist>(ArrayView&lt; T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a9e4a13e421a34a19eec002ed65bda582</anchor>
      <arglist>(StaticArrayView&lt; size_, T &gt;)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aaf76e645cbe62b360a0f13de4dff2837</anchor>
      <arglist>(T(&amp;)[size_])</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a58d7bb8d46f44949b57b226485db4a9b</anchor>
      <arglist>(T *data)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a3cf428b256a5ed8941145e4d15ae8b45</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aee41b58bdb45963c476ce7554dda2290</anchor>
      <arglist>(StaticArrayView&lt; size, T &gt; view)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aee9c43d476128f05403e638ce539c90d</anchor>
      <arglist>(T(&amp;data)[size])</arglist>
    </member>
    <member kind="function">
      <type>Utility::Debug &amp;</type>
      <name>enumSetDebugOutput</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a12c4ea794fa62c56969b1311e2b5c21b</anchor>
      <arglist>(Utility::Debug &amp;debug, EnumSet&lt; T, fullValue &gt; value, const char *empty, std::initializer_list&lt; T &gt; enums)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aa8111199cdb6cc30c470e3cc9c54a30e</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr ArrayView&lt; const T &gt;</type>
      <name>arrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a5b40a450ade4927381bf97a165e3621e</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a4f6c86de70a1ab496a1d798648c22f80</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr StaticArrayView&lt; size, const T &gt;</type>
      <name>staticArrayView</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ab15dc352f75f85e562eaa85ae058ac9f</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a5ebf447d7f388e81fe6d34910c80cfbe</anchor>
      <arglist>(StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>StaticArrayView&lt; size *sizeof(T)/sizeof(U), const U &gt;</type>
      <name>arrayCast</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>aae0b8c37cdca03924fac3a3ff457bf9b</anchor>
      <arglist>(const StaticArray&lt; size, T &gt; &amp;array)</arglist>
    </member>
    <member kind="function">
      <type>constexpr std::size_t</type>
      <name>arraySize</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a0cad6e09b65666040bb2460df883e4a3</anchor>
      <arglist>(const StaticArray&lt; size_, T &gt; &amp;)</arglist>
    </member>
    <member kind="variable">
      <type>constexpr DefaultInitT</type>
      <name>DefaultInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a10f8c95aaa0d51bb77976fe3e4924a5e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr ValueInitT</type>
      <name>ValueInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a26d49cbb15d20d7dc72878f522e24444</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr NoInitT</type>
      <name>NoInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a7094104336b357363773b3f29891d048</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr DirectInitT</type>
      <name>DirectInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>a69da0a827c69b45cec539ec0c201a119</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>constexpr InPlaceInitT</type>
      <name>InPlaceInit</name>
      <anchorfile>namespaceCorrade_1_1Containers.html</anchorfile>
      <anchor>ab86b6e81b5157279ebfbf2b716fb2d6d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Interconnect</name>
    <filename>namespaceCorrade_1_1Interconnect.html</filename>
    <class kind="class">Corrade::Interconnect::Connection</class>
    <class kind="class">Corrade::Interconnect::Emitter</class>
    <class kind="class">Corrade::Interconnect::Receiver</class>
    <class kind="class">Corrade::Interconnect::StateMachine</class>
    <class kind="class">Corrade::Interconnect::StateTransition</class>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a9ede79fbd60785ea83ffed32762d02ab</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), void(*slot)(Args...))</arglist>
    </member>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a7476864056b3ca1a98430d5caef0e4b4</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), Lambda slot)</arglist>
    </member>
    <member kind="function">
      <type>Connection</type>
      <name>connect</name>
      <anchorfile>namespaceCorrade_1_1Interconnect.html</anchorfile>
      <anchor>a0d313f017168b64b817565ecb7100299</anchor>
      <arglist>(EmitterObject &amp;emitter, Interconnect::Emitter::Signal(Emitter::*signal)(Args...), ReceiverObject &amp;receiver, void(Receiver::*slot)(Args...))</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::PluginManager</name>
    <filename>namespaceCorrade_1_1PluginManager.html</filename>
    <class kind="class">Corrade::PluginManager::AbstractManager</class>
    <class kind="class">Corrade::PluginManager::AbstractManagingPlugin</class>
    <class kind="class">Corrade::PluginManager::AbstractPlugin</class>
    <class kind="class">Corrade::PluginManager::Manager</class>
    <class kind="class">Corrade::PluginManager::PluginMetadata</class>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; LoadState &gt;</type>
      <name>LoadStates</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>a61b14635bc0c147c65e8be80260ca891</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>LoadState</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>aa240e935222a178e1acb5c0853f15547</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a38c300f4fc9ce8a77aad4a30de05cad8">NotFound</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3485912f892918a4ed6b357abad8e5a4">WrongPluginVersion</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ae90c7458dc20c6326d3a34071de85e3f">WrongInterfaceVersion</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ad6ab066e75221cc9a69bfede5f75347c">WrongMetadataFile</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3c062ee3a4f39445ef4ba44ada5bde3c">UnresolvedDependency</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a3729d9667c9ed82ae96b6174b288a3a5">LoadFailed</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a84a8921b25f505d0d2077aeb5db4bc16">Static</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a7381d487d18845b379422325c0a768d6">Loaded</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a5111e24c1ecc6266ce0de4b4dc42033b">NotLoaded</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ac7de13f47f40f470d0795d224221a577">UnloadFailed</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547ab651efdb98a5d6bd2b3935d0c3f4a5e2">Required</enumvalue>
      <enumvalue file="namespaceCorrade_1_1PluginManager.html" anchor="aa240e935222a178e1acb5c0853f15547a019d1ca7d50cc54b995f60d456435e87">Used</enumvalue>
    </member>
    <member kind="function">
      <type>Utility::Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>namespaceCorrade_1_1PluginManager.html</anchorfile>
      <anchor>a73b06a0cd1333798b796a2061dc6bedd</anchor>
      <arglist>(Utility::Debug &amp;debug, PluginManager::LoadState value)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::TestSuite</name>
    <filename>namespaceCorrade_1_1TestSuite.html</filename>
    <namespace>Corrade::TestSuite::Compare</namespace>
    <class kind="class">Corrade::TestSuite::Comparator</class>
    <class kind="class">Corrade::TestSuite::Comparator&lt; double &gt;</class>
    <class kind="class">Corrade::TestSuite::Comparator&lt; float &gt;</class>
    <class kind="class">Corrade::TestSuite::Tester</class>
  </compound>
  <compound kind="namespace">
    <name>Corrade::TestSuite::Compare</name>
    <filename>namespaceCorrade_1_1TestSuite_1_1Compare.html</filename>
    <class kind="class">Corrade::TestSuite::Compare::Around</class>
    <class kind="class">Corrade::TestSuite::Compare::Container</class>
    <class kind="class">Corrade::TestSuite::Compare::File</class>
    <class kind="class">Corrade::TestSuite::Compare::FileToString</class>
    <class kind="class">Corrade::TestSuite::Compare::Greater</class>
    <class kind="class">Corrade::TestSuite::Compare::GreaterOrEqual</class>
    <class kind="class">Corrade::TestSuite::Compare::Less</class>
    <class kind="class">Corrade::TestSuite::Compare::LessOrEqual</class>
    <class kind="class">Corrade::TestSuite::Compare::SortedContainer</class>
    <class kind="class">Corrade::TestSuite::Compare::StringToFile</class>
    <member kind="function">
      <type>Around&lt; T &gt;</type>
      <name>around</name>
      <anchorfile>namespaceCorrade_1_1TestSuite_1_1Compare.html</anchorfile>
      <anchor>a76165bdc14d6ddd8ba20356ffd11e2e5</anchor>
      <arglist>(T epsilon)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Utility</name>
    <filename>namespaceCorrade_1_1Utility.html</filename>
    <namespace>Corrade::Utility::Directory</namespace>
    <namespace>Corrade::Utility::String</namespace>
    <namespace>Corrade::Utility::System</namespace>
    <namespace>Corrade::Utility::Unicode</namespace>
    <class kind="class">Corrade::Utility::AbstractHash</class>
    <class kind="class">Corrade::Utility::AndroidLogStreamBuffer</class>
    <class kind="class">Corrade::Utility::Arguments</class>
    <class kind="class">Corrade::Utility::Configuration</class>
    <class kind="class">Corrade::Utility::ConfigurationGroup</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; bool &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; char32_t &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; double &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; float &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; int &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long double &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; long long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; short &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; std::string &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned int &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned long long &gt;</class>
    <class kind="struct">Corrade::Utility::ConfigurationValue&lt; unsigned short &gt;</class>
    <class kind="class">Corrade::Utility::Debug</class>
    <class kind="class">Corrade::Utility::Endianness</class>
    <class kind="class">Corrade::Utility::Error</class>
    <class kind="class">Corrade::Utility::Fatal</class>
    <class kind="class">Corrade::Utility::HashDigest</class>
    <class kind="class">Corrade::Utility::MurmurHash2</class>
    <class kind="class">Corrade::Utility::Resource</class>
    <class kind="class">Corrade::Utility::Sha1</class>
    <class kind="class">Corrade::Utility::Warning</class>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; ConfigurationValueFlag &gt;</type>
      <name>ConfigurationValueFlags</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a4d1891e33318b3189e012812127acd8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>std::integral_constant&lt; bool,(Implementation::HasMemberBegin&lt; T &gt;::value||Implementation::HasBegin&lt; T &gt;::value||Implementation::HasStdBegin&lt; T &gt;::value)&amp;&amp;(Implementation::HasMemberEnd&lt; T &gt;::value||Implementation::HasEnd&lt; T &gt;::value||Implementation::HasStdEnd&lt; T &gt;::value)&gt;</type>
      <name>IsIterable</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a43561aa129a33d6c7aa6cb828dacdf46</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>ConfigurationValueFlag</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>acac747502f9bde1bf73cbfbe661c780d</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da594be08882c8e9d5efb9eeb62f303744">Oct</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da92640bd72988395b326c888614f8937a">Hex</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da21234a0e100d74037a4da2e53f3200d7">Scientific</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility.html" anchor="acac747502f9bde1bf73cbfbe661c780da621e7b8ece62fecc55e883252ff2fbe7">Uppercase</enumvalue>
    </member>
    <member kind="function">
      <type>Debug &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a0cd339017bd5cf30f3374f370cd8f19c</anchor>
      <arglist>(Debug &amp;debug, Debug::Color value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>sleep</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a0e8ee1cd790d25f505ee1405ec15e238</anchor>
      <arglist>(std::size_t ms)</arglist>
    </member>
    <member kind="function">
      <type>To</type>
      <name>bitCast</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a3831cece822dfde455336664676f6867</anchor>
      <arglist>(const From &amp;from)</arglist>
    </member>
    <member kind="function">
      <type>To</type>
      <name>bitCast</name>
      <anchorfile>namespaceCorrade_1_1Utility.html</anchorfile>
      <anchor>a3831cece822dfde455336664676f6867</anchor>
      <arglist>(const From &amp;from)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Utility::Directory</name>
    <filename>namespaceCorrade_1_1Utility_1_1Directory.html</filename>
    <member kind="typedef">
      <type>Containers::EnumSet&lt; Flag &gt;</type>
      <name>Flags</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a9df63241a8c57cdd64316a82bc9713b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>Flag</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a60fc75369f2d2d700d1cc758261cef35</anchor>
      <arglist></arglist>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35ad6186b962ccb090e812467363c008e0a">SkipDotAndDotDot</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35ab5fc8c3926425785013c492ab8ecc22f">SkipFiles</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a9011abdfaf22b80f4ae98a3f59fed46b">SkipDirectories</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a2e58e2c745e14cc46a112c48075b863e">SkipSpecial</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a6e1549d1367460b4ab06f5575f2d427c">SortAscending</enumvalue>
      <enumvalue file="namespaceCorrade_1_1Utility_1_1Directory.html" anchor="a60fc75369f2d2d700d1cc758261cef35a12995c419287e0ef1f6f2d6ce1dfc12a">SortDescending</enumvalue>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>fromNativeSeparators</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a454691f5180e746e5f37d50e5aab9f13</anchor>
      <arglist>(std::string path)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>toNativeSeparators</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ae912c293390240c7c55cf36a3dd64fc3</anchor>
      <arglist>(std::string path)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>path</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a222baead7128ec0cadc433e3a8c0059f</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>filename</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a07df998aa551057785f2db6c51e78e63</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>join</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>aa1c57ae6d2c15c7507ad1ae70810d4de</anchor>
      <arglist>(const std::string &amp;path, const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>mkpath</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ad80859f373fbf1ed39b11eb27649c34b</anchor>
      <arglist>(const std::string &amp;path)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>rm</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>ac78c147e3378045ca84362dc1617a5d2</anchor>
      <arglist>(const std::string &amp;path)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>move</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>aa3505c07378a35988f43dca3496f9631</anchor>
      <arglist>(const std::string &amp;oldPath, const std::string &amp;newPath)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>fileExists</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a1a83e17a2230dd6c04d33dac2a6e745f</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isSandboxed</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a3cc7a87483e3b192253cbe72d1cb1f98</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>executableLocation</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a4e3dbc86c7b8d0c3c39d9802a9947b35</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>home</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a8dcea7e885d165d77f8ef68814788bd9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>configurationDir</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a73ddfdc8249b1bc779e8bb58cd9c319a</anchor>
      <arglist>(const std::string &amp;name)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>tmp</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a7b3ca5990decccd118261fe86824af70</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>list</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a27708f99ad155cdf20dbfb7926478582</anchor>
      <arglist>(const std::string &amp;path, Flags flags=Flags())</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; char &gt;</type>
      <name>read</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a4a9da22f34b0eb0f409b8183032b46db</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>readString</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>afdc9b2319f9470117c4fb6b2ecf63187</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>write</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>af3e40dd0b19be0b79e8d2bad631f9009</anchor>
      <arglist>(const std::string &amp;filename, Containers::ArrayView&lt; const void &gt; data)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>writeString</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a2e31a8897f1f4d78ec7179d584ab0100</anchor>
      <arglist>(const std::string &amp;filename, const std::string &amp;data)</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; char, MapDeleter &gt;</type>
      <name>map</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a769141becd7eed77b15c0d0d1a79cc04</anchor>
      <arglist>(const std::string &amp;filename, std::size_t size)</arglist>
    </member>
    <member kind="function">
      <type>Containers::Array&lt; const char, MapDeleter &gt;</type>
      <name>mapRead</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Directory.html</anchorfile>
      <anchor>a84c45342a8cec2111853f51873d82c9d</anchor>
      <arglist>(const std::string &amp;filename)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Utility::String</name>
    <filename>namespaceCorrade_1_1Utility_1_1String.html</filename>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a5dcd2f1e07dfeb6b3f523b8fd7fcc0db</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a13384c2eb91136a2282cabcf6d40e7e7</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ab1422c3f09858ed4959d7ddd022c913a</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ab0ad1369cf991d4a24ad9eb546da5994</anchor>
      <arglist>(const std::string &amp;string)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>split</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac33538e87b593ea3025b97cb9c4f9c7d</anchor>
      <arglist>(const std::string &amp;string, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a223a1c3ccc10a0a1c0d3cea7e1bb8770</anchor>
      <arglist>(const std::string &amp;string, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>join</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a9af3018407a9cac247c09da9ac8e577d</anchor>
      <arglist>(const std::vector&lt; std::string &gt; &amp;strings, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>joinWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac2adb8eafce8ddb6f7c163bd112e5c57</anchor>
      <arglist>(const std::vector&lt; std::string &gt; &amp;strings, char delimiter)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>lowercase</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a09fde2ac2c5c7a28e0c694d22ca89ee6</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>uppercase</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a4593d63bc7b1f340584b21d6a169afe9</anchor>
      <arglist>(std::string string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>fromArray</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a3e5a23b4c3afe46b19c0ea93a098e634</anchor>
      <arglist>(const char *string)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>fromArray</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>acb24339bd65dda380b696d363f4cc799</anchor>
      <arglist>(const char *string, std::size_t length)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>afc39c7e3c0d3da5bb4cfdf4bada808ab</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ltrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a9b120b72f24ac6c1266d60b112a131f6</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>aa7519775e721a492a57c3752b6e0a5ae</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>rtrim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a787500cae52742d206c30a00d315bd88</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a010626127bd50cc902768b3fa50e8e8c</anchor>
      <arglist>(std::string string, const std::string &amp;characters)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>trim</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a3e6ed45cb59343ff1c968cc7c5f74864</anchor>
      <arglist>(std::string string, const char(&amp;characters)[size])</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ac0ff6b03a281fecebe8540c3aa86cd47</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;delimiters)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>splitWithoutEmptyParts</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a1360ec5f0e3aa9a34af046480356292f</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;delimiters)[size])</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>beginsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>add091001179ac1455795365a3b667073</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;prefix)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>beginsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>aca63ac7667c9b71ce5e3240daf32316e</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;prefix)[size])</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>endsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>a810b5cb07e3a5e5063145ef47567ab7f</anchor>
      <arglist>(const std::string &amp;string, const std::string &amp;suffix)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>endsWith</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1String.html</anchorfile>
      <anchor>ace94f739418cde7292a1e4ffdb75598e</anchor>
      <arglist>(const std::string &amp;string, const char(&amp;suffix)[size])</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Utility::System</name>
    <filename>namespaceCorrade_1_1Utility_1_1System.html</filename>
    <member kind="function">
      <type>void</type>
      <name>sleep</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1System.html</anchorfile>
      <anchor>a7f93179c285d2667b97d9b38476d4cda</anchor>
      <arglist>(std::size_t ms)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>Corrade::Utility::Unicode</name>
    <filename>namespaceCorrade_1_1Utility_1_1Unicode.html</filename>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a23fbffa5a6184ab6c29030e152182c29</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text, std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ab5eed1f9f89a2e19eaef0c0f64fb9900</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text, std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::size_t</type>
      <name>utf8</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a0b1547a3b650117161826a75943e5946</anchor>
      <arglist>(char32_t character, Containers::StaticArrayView&lt; 4, char &gt; result)</arglist>
    </member>
    <member kind="function">
      <type>std::u32string</type>
      <name>utf32</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a738435f73e123822a70f692d9d9cfec8</anchor>
      <arglist>(const std::string &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a3cd253f620e843e9ecac1d427b7c070f</anchor>
      <arglist>(const std::string &amp;text, const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>nextChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a634c36aab0be25e8417905c1a1b492c4</anchor>
      <arglist>(const char(&amp;text)[size], const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ad231509de72398162b11d48f9ea75f85</anchor>
      <arglist>(const std::string &amp;text, const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::pair&lt; char32_t, std::size_t &gt;</type>
      <name>prevChar</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ab7b8f89831f52d67d311c5886b847ce6</anchor>
      <arglist>(const char(&amp;text)[size], const std::size_t cursor)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>af92cf003d4c9bf36b0a53777ab5a0445</anchor>
      <arglist>(const std::string &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>aa4f142c6268dbd30d3b816e36e9ab37b</anchor>
      <arglist>(Containers::ArrayView&lt; const char &gt; text)</arglist>
    </member>
    <member kind="function">
      <type>std::wstring</type>
      <name>widen</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a025f6762f095b8180ce0569573b82380</anchor>
      <arglist>(const char *text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>ad81ae601a86426189543698218fb276d</anchor>
      <arglist>(const std::wstring &amp;text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a68b980fd21405108c7eb80d1b07408e6</anchor>
      <arglist>(Containers::ArrayView&lt; const wchar_t &gt; text)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>narrow</name>
      <anchorfile>namespaceCorrade_1_1Utility_1_1Unicode.html</anchorfile>
      <anchor>a7dfc7f84250f3a9a16f4a8fb4bc7a97b</anchor>
      <arglist>(const wchar_t *text)</arglist>
    </member>
  </compound>
  <compound kind="page">
    <name>building-corrade</name>
    <title>Downloading and building</title>
    <filename>building-corrade</filename>
    <docanchor file="building-corrade" title="Downloading the sources">building-corrade-download</docanchor>
    <docanchor file="building-corrade" title="Compilation, installation">building-corrade-compilation</docanchor>
    <docanchor file="building-corrade" title="CMake primer">building-corrade-cmake</docanchor>
    <docanchor file="building-corrade" title="Via command-line (on Linux/Unix)">building-corrade-linux</docanchor>
    <docanchor file="building-corrade" title="Building on Windows">building-corrade-windows</docanchor>
    <docanchor file="building-corrade" title="Using Visual Studio">building-corrade-windows-msvc</docanchor>
    <docanchor file="building-corrade" title="Using QtCreator">building-corrade-windows-qtcreator</docanchor>
    <docanchor file="building-corrade" title="Enabling or disabling features">building-corrade-features</docanchor>
    <docanchor file="building-corrade" title="Building and running unit tests">building-corrade-tests</docanchor>
    <docanchor file="building-corrade" title="Building documentation">building-corrade-doc</docanchor>
    <docanchor file="building-corrade" title="Building examples">building-corrade-examples</docanchor>
    <docanchor file="building-corrade" title="Prepared packages">building-corrade-packages</docanchor>
    <docanchor file="building-corrade" title="ArchLinux packages">building-corrade-packages-arch</docanchor>
    <docanchor file="building-corrade" title="Gentoo ebuilds">building-corrade-packages-gentoo</docanchor>
    <docanchor file="building-corrade" title="RPM packages">building-corrade-packages-rpm</docanchor>
    <docanchor file="building-corrade" title="DEB packages">building-corrade-packages-deb</docanchor>
    <docanchor file="building-corrade" title="Homebrew formulas">building-corrade-packages-brew</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling">building-corrade-cross</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling for Windows RT">building-corrade-cross-winrt</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling for Windows using MinGW-w64">building-corrade-cross-win</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling for Emscripten">building-corrade-cross-emscripten</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling for iOS">building-corrade-cross-ios</docanchor>
    <docanchor file="building-corrade" title="Crosscompiling for Android ARM and x86">building-corrade-cross-android</docanchor>
    <docanchor file="building-corrade" title="Continuous Integration">building-corrade-ci</docanchor>
    <docanchor file="building-corrade" title="Travis">building-corrade-ci-travis</docanchor>
    <docanchor file="building-corrade" title="AppVeyor">building-corrade-ci-appveyor</docanchor>
  </compound>
  <compound kind="page">
    <name>corrade-cmake</name>
    <title>Usage with CMake</title>
    <filename>corrade-cmake</filename>
    <docanchor file="corrade-cmake" title="Other CMake modules">corrade-cmake-modules</docanchor>
    <docanchor file="corrade-cmake" title="Macros and functions">corrade-cmake-functions</docanchor>
    <docanchor file="corrade-cmake">corrade-cmake-add-test</docanchor>
    <docanchor file="corrade-cmake">corrade-cmake-add-resource</docanchor>
    <docanchor file="corrade-cmake">corrade-cmake-add-plugin</docanchor>
    <docanchor file="corrade-cmake">corrade-cmake-add-static-plugin</docanchor>
    <docanchor file="corrade-cmake">corrade-cmake-find-dlls-for-libs</docanchor>
  </compound>
  <compound kind="page">
    <name>corrade-example-index</name>
    <title>Examples and tutorials</title>
    <filename>corrade-example-index</filename>
  </compound>
  <compound kind="page">
    <name>corrade-coding-style</name>
    <title>Coding style</title>
    <filename>corrade-coding-style</filename>
    <docanchor file="corrade-coding-style" title="Text files in general">corrade-coding-style-text</docanchor>
    <docanchor file="corrade-coding-style" title="CMake code">corrade-coding-style-cmake</docanchor>
    <docanchor file="corrade-coding-style" title="C++ code">corrade-coding-style-cpp</docanchor>
    <docanchor file="corrade-coding-style" title="File and directory naming">corrade-coding-style-cpp-filesystem</docanchor>
    <docanchor file="corrade-coding-style" title="File structure">corrade-coding-style-cpp-files</docanchor>
    <docanchor file="corrade-coding-style" title="Code format">corrade-coding-style-cpp-format</docanchor>
    <docanchor file="corrade-coding-style" title="Naming">corrade-coding-style-cpp-naming</docanchor>
    <docanchor file="corrade-coding-style" title="Forward declarations and forward declaration headers">corrade-coding-style-cpp-forward-declarations</docanchor>
    <docanchor file="corrade-coding-style" title="Namespace declarations">corrade-coding-style-cpp-namespace</docanchor>
    <docanchor file="corrade-coding-style" title="Class and structure declarations">corrade-coding-style-cpp-classes</docanchor>
    <docanchor file="corrade-coding-style" title="Blocks, whitespace and indentation">corrade-coding-style-cpp-style</docanchor>
    <docanchor file="corrade-coding-style" title="Switch statements">corrade-coding-style-cpp-switches</docanchor>
    <docanchor file="corrade-coding-style" title="Class member and function keywords">corrade-coding-style-cpp-keywords</docanchor>
    <docanchor file="corrade-coding-style" title="Preprocessor macros">corrade-coding-style-cpp-macros</docanchor>
    <docanchor file="corrade-coding-style" title="Class constructors and destructors">corrade-coding-style-cpp-constructors</docanchor>
    <docanchor file="corrade-coding-style" title="Constant expressions and constants">corrade-coding-style-cpp-constexpr</docanchor>
    <docanchor file="corrade-coding-style" title="Assertions">corrade-coding-style-cpp-assert</docanchor>
    <docanchor file="corrade-coding-style" title="Enums and inheritance">corrade-coding-style-cpp-enum-inheritance</docanchor>
    <docanchor file="corrade-coding-style" title="Naked pointers">corrade-coding-style-cpp-pointers</docanchor>
    <docanchor file="corrade-coding-style" title="Forbidden and deprecated C/C++ features">corrade-coding-style-cpp-forbidden</docanchor>
    <docanchor file="corrade-coding-style" title="Heavy STL headers">corrade-coding-style-cpp-heavy-headers</docanchor>
    <docanchor file="corrade-coding-style" title="using namespace keyword">corrade-coding-style-cpp-using</docanchor>
    <docanchor file="corrade-coding-style" title="C-style casts">corrade-coding-style-cpp-backward</docanchor>
    <docanchor file="corrade-coding-style" title="Unscoped and untyped enums">corrade-coding-style-cpp-enums</docanchor>
    <docanchor file="corrade-coding-style" title="static keyword">corrade-coding-style-cpp-anonymous-namespace</docanchor>
    <docanchor file="corrade-coding-style" title="Comments">corrade-coding-style-comments</docanchor>
    <docanchor file="corrade-coding-style" title="Doxygen documentation">corrade-coding-style-documentation</docanchor>
    <docanchor file="corrade-coding-style" title="Section ordering">corrade-coding-style-documentation-ordering</docanchor>
    <docanchor file="corrade-coding-style" title="Special documentation commands">corrade-coding-style-documentation-commands</docanchor>
    <docanchor file="corrade-coding-style" title="Documentation-related TODOs">corrade-coding-style-documentation-commands-todoc</docanchor>
    <docanchor file="corrade-coding-style" title="Debugging operators">corrade-coding-style-documentation-commands-debugoperator</docanchor>
    <docanchor file="corrade-coding-style" title="Configuration value parsers and writers">corrade-coding-style-documentation-commands-configurationvalue</docanchor>
    <docanchor file="corrade-coding-style" title="Partially supported features">corrade-coding-style-documentation-commands-partialsupport</docanchor>
    <docanchor file="corrade-coding-style" title="Backwards compatibility and experimental features">corrade-coding-style-documentation-commands-experimental</docanchor>
    <docanchor file="corrade-coding-style" title="Git commit messages, branch and tag format">corrade-coding-style-git</docanchor>
  </compound>
  <compound kind="page">
    <name>corrade-changelog</name>
    <title>Changelog</title>
    <filename>corrade-changelog</filename>
    <docanchor file="corrade-changelog" title="Changes since 2015-05 snapshot">corrade-changelog-latest</docanchor>
    <docanchor file="corrade-changelog" title="2015-05 snapshot">corrade-changelog-2015-05</docanchor>
    <docanchor file="corrade-changelog" title="2014-06 snapshot">corrade-changelog-2014-06</docanchor>
    <docanchor file="corrade-changelog" title="2014-01 snapshot">corrade-changelog-2014-01</docanchor>
    <docanchor file="corrade-changelog" title="2013-10 snapshot">corrade-changelog-2013-10</docanchor>
    <docanchor file="corrade-changelog" title="2013-08 snapshot">corrade-changelog-2013-08</docanchor>
    <docanchor file="corrade-changelog" title="Initial release">corrade-changelog-initial</docanchor>
  </compound>
  <compound kind="page">
    <name>interconnect</name>
    <title>Signals and slots</title>
    <filename>interconnect</filename>
    <docanchor file="interconnect" title="Implementing signals">interconnect-signals</docanchor>
    <docanchor file="interconnect" title="Implementing slots">interconnect-slots</docanchor>
    <docanchor file="interconnect" title="Connecting signals to slots">interconnect-connecting</docanchor>
    <docanchor file="interconnect" title="Emitting signals">interconnect-emitting</docanchor>
    <docanchor file="interconnect" title="Compiling and running the example">interconnect-compiling</docanchor>
  </compound>
  <compound kind="page">
    <name>plugin-management</name>
    <title>Plugin management tutorial</title>
    <filename>plugin-management</filename>
    <docanchor file="plugin-management" title="Plugin interface">plugin-management-interface</docanchor>
    <docanchor file="plugin-management" title="Plugin definition">plugin-management-plugin</docanchor>
    <docanchor file="plugin-management" title="Plugin compilation">plugin-management-compilation</docanchor>
    <docanchor file="plugin-management" title="Plugin management">plugin-management-management</docanchor>
  </compound>
  <compound kind="page">
    <name>resource-management</name>
    <title>Resource management tutorial</title>
    <filename>resource-management</filename>
    <docanchor file="resource-management" title="Resource compilation">resource-management-compilation</docanchor>
    <docanchor file="resource-management" title="Resource management">resource-management-management</docanchor>
  </compound>
  <compound kind="page">
    <name>testsuite</name>
    <title>Testing and benchmarking with TestSuite</title>
    <filename>testsuite</filename>
    <docanchor file="testsuite" title="Tester class">testsuite-class</docanchor>
    <docanchor file="testsuite" title="Compilation and running">testsuite-compilation</docanchor>
  </compound>
  <compound kind="page">
    <name>index</name>
    <title></title>
    <filename>index</filename>
    <docanchor file="index" title="Supported platforms">corrade-mainpage-platforms</docanchor>
    <docanchor file="index" title="Features">corrade-mainpage-features</docanchor>
    <docanchor file="index" title="Building Corrade">corrade-mainpage-building</docanchor>
    <docanchor file="index" title="Examples and tutorials">corrade-mainpage-getting-started</docanchor>
    <docanchor file="index" title="Hacking Corrade">corrade-mainpage-hacking</docanchor>
    <docanchor file="index" title="License">corrade-mainpage-license</docanchor>
  </compound>
</tagfile>
