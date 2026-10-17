.. _reader-compatibility:

Reader compatibility options
============================

libCZI normally interprets each sub-block as a two-dimensional X-Y bitmap on
a plane selected by coordinates such as Z, C and T. Some acquisition workflows
use an alternative dimension convention: a sub-block contains an X-T or X-Z
image, with T or Z describing its rows. Reader compatibility options allow
these images to be accessed through libCZI's existing bitmap and accessor APIs.

These options are explicit choices made when opening a document. The default
interpretation remains unchanged, and libCZI does not automatically detect
which convention an acquisition used.

Why a dimension swap is useful
----------------------------------------

The motivating example was a confocal line-scan acquisition: one spatial line
was scanned repeatedly over time, and each sub-block contained several of
these lines. ZEN displayed the complete X-T image, while reading the same data
through libCZI exposed only the first line of each sub-block. The original
report is available in `MEXlibCZI issue #16
<https://github.com/ptahmose/MEXlibCZI/issues/16>`_.

This reflects a difference in how acquisition dimensions are represented and
interpreted. In libCZI's usual image model, T selects a plane rather than
describing rows within a bitmap. Treating the rows as Y makes the complete
X-T image available to the existing image-processing operations. The same
approach can be used for X-Z sub-blocks.

The workaround adapts the reader's view of the data; it does not rewrite the
document or its XML metadata.

Dimension mapping
-----------------

Set ``ICZIReader::OpenOptions::dimension_swap_workaround`` to one of:

.. list-table::
   :header-rows: 1
   :widths: 25 40 35

   * - Value
     - Bitmap rows (Y)
     - Remaining plane coordinate
   * - ``None`` (default)
     - Original Y
     - Original T and Z
   * - ``XtSubblocks``
     - Original T start, logical size and stored size
     - Original Y becomes T; Z is unchanged
   * - ``XzSubblocks``
     - Original Z start, logical size and stored size
     - Original Y becomes Z; T is unchanged

X is unchanged. The swap is applied to both sub-block directory entries and
sub-block headers. It affects the information exposed by enumeration, reader
statistics, sub-block reads and accessors. Choose either the XT or the XZ
workaround; both cannot be selected at once.

.. note::

   After the swap, bitmap rows represent time samples or Z positions, rather
   than spatial Y positions. Interpret row coordinates and physical units
   accordingly. XML metadata retains its original dimension descriptions;
   the workaround does not generate new calibration or timing information.

Opening and composing an X-T image
----------------------------------------

Pass the options to ``ICZIReader::Open`` before reading the directory:

.. code-block:: cpp

   auto stream = libCZI::CreateStreamFromFile(L"linescan.czi");
   auto reader = libCZI::CreateCZIReader();
   libCZI::ICZIReader::OpenOptions options;
   options.dimension_swap_workaround =
       libCZI::ICZIReader::OpenOptions::DimensionSwapWorkaround::XtSubblocks;
   reader->Open(stream, &options);

For X-Z data, select ``XzSubblocks`` instead. Inspect ``GetStatistics()`` and
the enumerated coordinates after opening: the reported bounding box uses
the swapped row coordinates, and the plane coordinates use the mapping above.

For example, if the mapped document has only channel C0 and time coordinate
T0 (from the original Y coordinate), its image can be composed as follows:

.. code-block:: cpp

   const auto statistics = reader->GetStatistics();
   libCZI::CDimCoordinate plane{
       {libCZI::DimensionIndex::C, 0},
       {libCZI::DimensionIndex::T, 0}
   };
   auto accessor = reader->CreateSingleChannelTileAccessor();
   auto image = accessor->Get(statistics.boundingBox, &plane, nullptr);

Choose the plane from the dimensions actually reported by the reader; C0T0
is an example, not a universal selection. For large acquisitions, request
smaller regions instead of composing the complete bounding box in memory.

Directory and header differences
----------------------------------------

Some affected acquisitions also have differences between the sub-block
directory and sub-block headers. Handling those differences is a separate
choice from swapping dimensions. The default is to use the directory's
information and report a discrepancy when the sub-block is read.

If examination of an affected document shows that it needs the directory's
interpretation despite a header difference, discrepancy reporting can be
disabled explicitly:

.. code-block:: cpp

   using Policy = libCZI::ICZIReader::OpenOptions::SubBlockDirectoryInfoPolicy;
   options.subBlockDirectoryInfoPolicy =
       Policy::SubBlockDirectoryPrecedence | Policy::IgnoreDiscrepancy;
   // Set this before calling reader->Open(stream, &options).

``SubBlockHeaderPrecedence`` instead selects the header's information.
Changing precedence alone does not disable discrepancy reporting: combine
it with ``IgnoreDiscrepancy`` if that is also intended.

Ignoring a discrepancy does not establish which source is correct and does
not repair the file. Select these settings for documents whose acquisition
convention is understood, rather than applying them to all files. See
:doc:`resolution_protocol` for the default interpretation rules.

Passing options as a string
----------------------------------------

``libCZI::StringToOpenOptions`` parses semicolon-separated ``key=value`` pairs.
This is useful for wrappers and is also the format accepted by CZIcmd's
``--open-options`` argument:

.. code-block:: cpp

   auto options = libCZI::StringToOpenOptions(
       "dimensionSwapWorkaround=XtSubblocks;"
       "subBlockDirectoryInfoDiscrepancy=ignore");
   reader->Open(stream, &options);
   auto serialized = libCZI::OpenOptionsToString(options);

The example explicitly enables both the XT swap and ignored discrepancies;
omit the second pair when discrepancy reporting should remain enabled.

.. list-table:: Supported string keys
   :header-rows: 1
   :widths: 35 45 20

   * - Key
     - Values
     - Default
   * - ``dimensionSwapWorkaround``
     - ``None``, ``XtSubblocks``, ``XzSubblocks``
     - ``None``
   * - ``subBlockDirectoryInfoPrecedence``
     - ``SubBlockDirectoryPrecedence``, ``SubBlockHeaderPrecedence``
     - ``SubBlockDirectoryPrecedence``
   * - ``subBlockDirectoryInfoDiscrepancy``
     - ``error``, ``ignore``
     - ``error``
   * - ``laxSubblockCoordinateChecks``
     - ``true``, ``false``
     - ``true``
   * - ``ignoreSizeMForPyramidSubblocks``
     - ``true``, ``false``; relevant only with strict coordinate checks
     - ``false``
   * - ``defaultFrameOfReference``
     - ``Invalid``, ``Default``, ``RawSubBlockCoordinateSystem``,
       ``PixelCoordinateSystem``
     - ``Invalid``

``Invalid`` and ``Default`` resolve to the raw sub-block coordinate system.
The frame-of-reference setting is independent of the dimension swap.

Keys and values are case-sensitive. Whitespace around pairs, keys and values
is ignored. Empty input and omitted keys retain the defaults. Unknown or
duplicate keys, invalid values, empty pairs (including a trailing semicolon),
malformed pairs and null input cause ``std::invalid_argument``. Escaping is
not supported. ``OpenOptionsToString`` produces all options in a canonical
order and rejects enum values or policy bits it cannot represent.

For command-line examples, see :ref:`czicmd-reader-options`.
