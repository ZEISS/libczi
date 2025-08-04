#ifndef JXRLIB_SYMBOL_MANGLE_H
#define JXRLIB_SYMBOL_MANGLE_H

// Macro to prefix all public JXR-related functions to avoid symbol collisions.
// This enables multiple libraries using JXR-based code to coexist safely.

#ifndef JXRLIB_API
#define JXRLIB_API(name) libCZIjxrlib_##name
#endif

#endif // JXRLIB_SYMBOL_MANGLE_H
