resolution protocol                 {#resolution_protocol_}
===================

# Resolution Protocol for Ambiguous or Contradictory Information

## Introduction

In cases where a CZI file contains ambiguous or contradictory information, the behavior
described in this document shall be considered canonical. Implementations MUST resolve
any conflicts according to these guidelines to ensure consistency and interoperability.  
Note that those guidelines only give recommendations for how the data is to be interpreted, this does not mean that
one should rely on this. 
A CZI containing mismatches (as discussed below) is without a doubt to be considered *malformed* and *corrupt*. However - if facing such
a malformed file, then a consistent behavior is desirable. Also, it is impractical to check internal consistency in all aspects before
reading data from a CZI-file - it is designed in way that it allows to access only the parts of the file which are of interest (as opposed
to be read in its entirety to memory). If such discrepancies are encountered, it is strongly recommended to report them in some form,
potentially even to the user. Those cases are indications of a malformed and corrupted file, and should be treated as such.

Note that by defining which piece of information is to be considered as authoritative, no statement is made whether it is _correct_. The whole
purpose here is to have _consistent_ behavior when facing malformed data. Following these procedures does not magically make the underlying problem
go away (which usually is either a problem when authoring the CZI or some sort of storage malfunction or error in transmission).

## discrepancies between sub-block's DirectoryEntry and the actual payload content

In a sub-block, we have a description of the payload content in the DirectoryEntry as well as the actual payload content.
Physically, the information is represented by the data-structure [SubBlockDirectoryEntryDV](https://github.com/ZEISS/libczi/blob/11015ae9aa97abbf9d78293a27115393077f9146/Src/libCZI/CziStructs.h#L175).
The information in this structure which is relevant for the payload content is:
* Pixel Type
* Compression
* Physical width and height of the bitmap

It is now conceivable that the information in the DirectoryEntry is not consistent with the actual payload content. The following cases need to be considered:

1) If Compression is "uncompressed", then the expected size of the payload content can be calculated. In this case, a mismatch between the actual payload size and the expected size (calculated from pixel type and width and height) is possible.
2) If Compression is JPGXR, then the compressed data (in the payload) also contains information about the width and height of the bitmap. Or - when the payload content is decompressed, its width and height is determined (from the compressed data), and there might be a mismatch between this size and the width/height given in the DirectoryEntry.

Note that in cases with Compression other than JPGXR, e.g. zstd0 or zstd1, there is no redundant specification of the width/height in the compressed data. For zstd0/zstd1 compression, the decompression gives an unstructured blob of data, only described by its size. This case is therefore to be treated similar to case 1).

### case 1: payload-data size mismatch for uncompressed subblocks

The information in the DirectoyEntry is to be considered authoritative, i.e. the data is to be interpreted as a bitmap with width/height and pixel type as specified in the DirectoryEntry, and with a minimal stride.  

If the payload size is larger than what is calculated from width, height and pixel type, the additional data is to be discarded.   

If the payload size is smaller than what is calculated from width, height and pixel type, then the missing data is to be filled with zeros.


### case 2: width/height/pixel type size mismatch for JPGXR-data

If the bitmap characteristics (i.e. width, height and pixel type) of the decompressed bitmap and the information in the DirectoryEntry are contradicting, then the DirectoryEntry information is to be considered authoritative. The following measures are recommended:

* If width or height is different, then the decompressed bitmap is to be cropped or padded to give the width/height as specified in the DirectoryEntry.
* Cropping or padding is done so that the top-left pixel (of the decompressed bitmap) remains at the top-left position.
* If padding is necessary, the enlarged bitmap is to be filled with zeroes.
* If the pixel type is different, then a conversion into the pixel type as given in the DirectoryEntry can be considered. Otherwise, the resulting bitmap is all zeroes.


## discrepancies between sub-block's DirectoryEntry in SubBlockDirectory-segment and the actual Subblock-segment

The information contained in the DirectoryEntry structure is redundantly present in a CZI - it is given in the SubBlockDirectory, and in the actual subblock-segment.
In case of a mismatch between those two pieces of information, generally the copy in the SubBlockDirectory is to be seen as authoritative.   

