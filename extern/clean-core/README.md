# clean-core
clean-core (`cc`) is a clean and lean reimagining of the C++ standard library.

## Goals

* significantly faster to compile than `std`
* forward declaration for all public types
* no slower than `std`
* safer than `std`
* more modular header design (each type can be separately included)
* convenient interfacing with code using `std` types
* removal of unintuitive behavior (e.g. `vector<bool>` or `optional::operator bool`)
* mostly keeping naming scheme and intent of `std`
* better debugging support and performance
* no dependency on `exception`s

## Requirements / Dependencies

* a C++17 compiler
* a few `std` features (that are hard to otherwise implement)

## Allowed `std` includes

* `<cstring>` (1ms, for memcpy)
* `<cstddef>` (1ms, for some types)
* `<type_traits>` (5-8ms, many not implementable ourselves)
* `<utility>` (10-15ms, swap, declval, NOTE: `cc` has own move and forward)
* `<atomic>` (20ms, hard to implement ourselves)

## Notable Changes

Changes that were rarely used features that increased implementation cost immensely:

* no `allocator`s
* no custom deleters for `unique_ptr`

Error-prone and unintuitive or suprising features:

* no specialized `vector<bool>`
* no `operator bool` for `optional<T>`
* no `operator<` for `optional<T>`

Others:

* no strong `exception` support
* no iterator-pair library, only ranges
* no unreadable `_UglyCase` (leaking non-caps macros is a sin)
* traits types and values are not suffixed with `_t` or `_v`
* no `volatile` support
* no `unique_ptr<T[]>`
* some interfaces are stricter to prevent easy mistakes

## New Features

* `span` (strided array view)
* `flat_` containers
* `inline_` types (no heap allocs)
* customizable low-impact `assert`
* internal assertions (optional)
    * bound-checked containers
    * null checks for smart pointer
    * contract checks

## TODO

* big list of comparison between `std` and `cc`
    * name of feature/class (e.g. `pair`)
    * header name of `std`/`cc`
    * parse time `std`/`cc`
    * preprocessed, significant LOC of `std`/`cc`
