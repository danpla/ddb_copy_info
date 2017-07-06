
## ddb_copy_info

ddb_copy_info is a plugin for DeadDBeeF music player that adds a menu
entry to copy information about selected tracks to the clipboard using
a custom format. This is the same as Copy name(s) function from
foobar2000.

The format string can be changed in the plugin's preferences
(Edit → Preferences → Plugins → Copy info → Configure).
The default one is `%tracknumber%. %artist% - %title% (%length%)`.


### Download

You can download binaries from
[the DeaDBeeF's website](http://deadbeef.sourceforge.net/plugins.html).
Packages contain both GTK 2 and GTK 3 versions of the plugin.


### Building from sources

You can compile the plugin either with Make or CMake. Both systems
create `gtk2` and `gtk3` build targets.

Before compiling, ensure you have DeaDbeeF's API headers
(`deadbeef/deadbeef.h` and `deadbeef/gtkui_api.h`). Both build systems
provide a `DEADBEEF_INC` option to add extra search paths.

Obviously, you'll also need a dev package of GTK+.


#### Make

Makefile provides the following options:

*   `RELEASE` — if 1, compile in release mode (add `-O2 -DNDEBUG`
    to `CFLAGS`) instead of debug. Default is 0.
*   `DEADBEEF_INC` — list of additional include paths (`-I`) to
    search for DeaDBeeF headers. Empty by default.

Simplest case. Build both GTK 2 and GTK 3 versions of the plugin in
debug mode:

```sh
cd ddb_copy_info
make
```

More advanced case. Build only GTK 2 version in release mode,
providing an additional include path (`/opt/deadbeef/include`):

```sh
cd ddb_copy_info
make gtk2 RELEASE=1 DEADBEEF_INC=-I/opt/deadbeef/include
```


#### CMake

CMake will not create a build target if the required version of GTK
will not be found.

Use `DEADBEEF_INC` option to provide a list of additional include
paths to search for DeaDBeeF headers. The paths should be separated
by semicolon and should not contain `-I`.

Simplest case. Build both GTK 2 and GTK 3 versions of the plugin:

```sh
cd ddb_copy_info
mkdir build && cd build

cmake ..
make
```

More advanced case. Build only GTK 2 version in release mode,
providing an additional include path (`/opt/deadbeef/include`):

```sh
cd ddb_copy_info
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release -DDEADBEEF_INC=/opt/deadbeef/include ..
make gtk2
```


### Installing


To install plugins, copy them to `~/.local/lib/deadbeef/`.
