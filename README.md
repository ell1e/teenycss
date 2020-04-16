
# teenycss - a header-only CSS library

teenycss is a library to parse a subset of Cascading Style Sheets,
as an easy header-only, self-contained library.


## Why?

teenycss is meant for use when a full web engine is excessive.
E.g. styling in UI toolkits, games, e-book readers, and more.

This library is preferrable over a web engine for these reasons:

- It's small with no dependencies. This means less integration
  work and less attack surface than a full web engine.

- No HTML/web assumptions that tags like `b`, `div`, ... exist,
  or that you even use a particular type of document object model.
  It is ideal for use of styling of not-strictly-web elements.

- It can be included as a single header file. This avoids
  introducing another build system you need to deal with for
  cross-compilation or similar.

This library is **not** preferrable if:

- You just want to throw in an existing HTML/CSS UI where the
  library / web view handles the DOM for you

- You want to parse the CSS of real-world, modern web page.
  **teenycss is not a complete implementation, web pages will
  render wrong.**


## Usage

### Include in Your Project

If you use the latest release, just grab the `teenycss.h` header file
and include it:

```c
#define TEENYCSS_IMPLEMENTATION  // define ONLY IN ONE OBJECT FILE.
#include <teenycss.h>
```
(the `#define` controls which ones of your object files contains the
actual function code. If you include it only in one of your files,
just always put the define.)

For a development version directly from git, build the `teenycss.h`
file from the separate source files using `make`.


### Usage Example

```c
teenycss_ruleset *ruleset = teenycss_Parse(
    "body {font-size:12px;}\n"
    "h1 {font-size:15px;}"
);
```

## License

teenycss is free software and it doesn't require attribution
outside of the text in the header file, [check the license for
details](LICENSE.md).
