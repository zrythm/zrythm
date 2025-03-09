## Version 0.2.4

* Minimum CMake version is now 3.5 (#156).
* Better module support (#151).
* Prevent dangling double-wrapping of `function_ref` (#154).
* Use EBO for `strong_typedef` on MSVC (#159).
* Add `flag_set::from_int` (#161).

## Version 0.2.3

* Enable the use of `import std` (#141, #147).
* Make `strong_typedef` structural so it can be used as NTTP (#126).
* Add safe integer comparisons (#134).
* Enable EBO on MSVC (#128).
* Various bugfixes and CMake improvements (#132, #133, #137, #139, #143, #146, #148).

## Version 0.2.2

* Replace `TYPE_SAFE_ARITHMETIC_UB` CMake option by `TYPE_SAFE_ARITHMETIC_POLICY` to enable checked arithmetic by default (#106)
* Be less strict in the signed/unsigned conversion of `integer` (#104)
* Various bugfixes (#97, #108, #111, #115)

## Version 0.2.1

This release is mainly bugfixes:

* Added `explicit_bool` strong typedef
* Fixed forwarding bug in `with()` of `array_ref`
* Silenced some warnings
* Improved CMake configuration

## Version 0.2

This release took a long time, so here are just the most important changes.

* Added `downcast()` utility function
* Fixed GCC 4.8 and clang support
* Improved documentation
* Improved CMake and added Conan support
* Hashing support for `boolean`, `integer`, `floating_point` and `strong_typedef`

## Vocabulary Types

* Added `object_ref<T>` and and `xvalue_ref<T>`
* Added `array_ref<T>`
* Added `function_ref<T>`
* Added `flag_set<T>`
* Breaking: renamed `distance_t` to `difference_t`
* Improved `index_t` and `difference_t`

### Optional & Variant:

* Breaking: `optional` now auto-unwraps, removed `unwrap()` function
* Breaking: `optional_ref` creation function now `opt_ref()` instead of `ref()`, but more than before
* Breaking: Removed `optional_ref<T>::value_or()`
* Added `optional_for<T>` utility typedef
* Added support for additional parameters to `with()` and make it more optimizer friendly
* Added `with()` for `tagged_union`
* Improved map functions: additional arguments, member function pointers, `void` returning function objects
* Bugfix for trivial variant copy/move/destroy

### Type Safe Building Blocks:

* Added pointer like access to `constrained_type`
* Added `constrained_ref`
* Added `throwing_verifier` for `constrained_ref` along with `sanitize()` helper functions
* Added literal operator to create a static bound of `bounded_type`
* Added bitwise operators for `strong_typedef`
* Added `constexpr` support to `strong_typedef`
* Added support for mixed operators in `strong_typedef`
* Made `constrained_type` `constexpr`
* Fixed strong typedef `get()` for rvalues


Thanks to @johelgp, @xtofl, @nicola-gigante, @BRevzin, @verri, @lisongmin, @Manu343726, @MandarJKulkarni, @gerboengels.
