Valid-Pixel-Mask Concept
========================


Introduction
------------

With the term *valid-pixel-mask* (or *mask* for short) we refer to a binary mask which indicates which pixels in a sub-block are valid and which are not. 
Basically, a sub-block in a CZI document is a rectangular image, and there is no way to indicate that some pixels in that rectangle are not valid. It is
therefore not possible to represent non-rectangular images directly. The valid-pixel-mask is a way to overcome this limitation.   
The concept is:

* An additional bitonal (i.e. 1 bit per pixel) bitmap is stored with each sub-block.
* A **zero** in this mask bitmap marks the corresponding pixel in the sub-block as **not valid**, a **one** declares the pixel as **valid**.
* For a multi-tile composition, non-valid pixels are not rendered.

Mask in pyramids
----------------

The immediate use-case for this mask-concept is with pyramids. Pyramids with CZI are constructed **per scene**, and the axis-aligned bounding-box of scenes may be overlapping - as shown here: 

.. image:: ../_static/images/overlapping_scenes.jpg
   :alt: overlapping scenes

Although the tiles of different scenes are not overlapping, the axis-aligned bounding-boxes of the scenes evidently are. This means that pyramid-tiles **are overlapping**, and it is then impossible to render a scene-composite which would give the complete image data. The result is then a composition like this: 

.. image:: ../_static/images/scene-composite-wo_mask.png
   :alt: composition without mask

Therefore, the pyramid-tiles of each scene can be provided with a valid-pixel-mask, and the composition then looks like this:

.. image:: ../_static/images/scene-composite-wo_mask.png
   :alt: composition with mask
