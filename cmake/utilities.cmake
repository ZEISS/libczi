# SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

#[=======================================================================[
BoolToYesNo
-----------

Converts a boolean variable to a "yes"/"no" string representation.

Synopsis
^^^^^^^^

.. code-block:: cmake

  BoolToYesNo(<var> <result_text>)

Description
^^^^^^^^^^^

This function takes a boolean variable and converts it to a human-readable
"yes" or "no" string. This is useful for displaying configuration options
in a user-friendly format.

Parameters
^^^^^^^^^^

``<var>``
  The boolean variable to convert. Can be any CMake variable that evaluates
  to true or false (TRUE, FALSE, ON, OFF, YES, NO, 1, 0, etc.).

``<result_text>``
  The name of the variable that will receive the result string.
  Will be set to "yes" if the input variable is true, "no" if false.
  The result is set in the parent scope.

Examples
^^^^^^^^

.. code-block:: cmake

  set(BUILD_TESTS TRUE)
  BoolToYesNo(BUILD_TESTS tests_enabled_text)
  message("Build tests: ${tests_enabled_text}")  # Output: "Build tests: yes"

  set(ENABLE_DOCS OFF)
  BoolToYesNo(ENABLE_DOCS docs_enabled_text)
  message("Enable docs: ${docs_enabled_text}")  # Output: "Enable docs: no"

#]=======================================================================]
function(BoolToYesNo var result_text)
  if (${var})
    set( ${result_text} "yes" PARENT_SCOPE)
  else()
    set( ${result_text} "no" PARENT_SCOPE)
  endif()
endfunction()