# This is meant to easily run the sphinx documentation generation
# on host systems that don't have the required dependencies installed.
# Note: the doxygen builds for the aggregated paths must be run
#       manually before the sphinx build.
# Example:
# 1. With a terminal (working directory in the path where this file resides in) run
#     ,----
#     | docker build -t custom-sphinx-image .
#     `----
#    to create a custom image that comprises the required dependencies for
#    the sphinx documentation generation of this repository.
#
# 2. From anywhere, assuming the clone of this repository on the host
#    machine is "d:/repos/libCZI-C_API", run
#     ,----
#     | docker run -it --rm -v "d:/repos/libCZI-C_API:/repo" custom-sphinx-image sphinx-build -M html repo/docs/source repo/docs/build
#     `----
#    This will create a self contained documentation build on the host in
#    "d:/repos/libCZI-C_API/docs/build"

# Use an official Python runtime as a parent image
FROM python:3.9-slim-buster

# Install git and other dependencies
RUN apt-get update && apt-get install -y --no-install-recommends git && rm -rf /var/lib/apt/lists/*

# Install Sphinx some extensions
RUN pip install --no-cache-dir sphinx==8.2.3 breathe==4.36.0 exhale==0.3.7 sphinx-book-theme==1.1.4 recommonmark==0.7.0 sphinx-markdown-tables==0.0.15 \
&& pip install --no-cache-dir -e "git+https://github.com/mithro/sphinx-contrib-mithro#egg=sphinx-contrib-exhale-multiproject&subdirectory=sphinx-contrib-exhale-multiproject"

# Add a non-root user after installing dependencies and switch to it
RUN useradd -ms /bin/bash sphinxuser
USER sphinxuser
