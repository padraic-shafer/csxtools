from typing import Literal, Union

import dask.array as da
from dask.array import Array as DaskArray


def rotate90(images: DaskArray, sense: Union[Literal["cw"], Literal["ccw"]] = "cw") -> DaskArray:
    """
    Rotate images by 90 degrees using Dask.
    This whole function is a moot wrapper around `da.rot90` from Dask, but written
    explicitly to match the old C code.

    Parameters
    ----------
    images : da.Array
        Input Dask array of images to rotate of shape (N, y, x),
        where N is the number of images and y, x are the image dimensions.
    sense : str, optional
        'cw' to rotate clockwise, 'ccw' to rotate anticlockwise.
        Default is 'cw'.

    Returns
    -------
    da.Array
        The rotated images as a Dask array.
    """
    # Rotate images. The axes (1, 2) specify the plane of rotation (y-x plane for each image).
    # k controls the direction and repetitions of the rotation.
    if sense == "ccw":
        k = 1
    elif sense == "cw":
        k = -1
    else:
        raise ValueError("sense must be 'cw' or 'ccw'")
    rotated_images = da.rot90(images, k=k, axes=(-2, -1))
    return rotated_images
