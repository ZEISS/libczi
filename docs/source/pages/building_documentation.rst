Documentation Generation
========================
This guide provides instructions for generating the documentation for the libCZI and libCZI-API libraries using Doxygen and Sphinx. For the latter, the source files are located in the docs/source folder.
Each libCZI and API has a separate Doxyfile. The reason for having separate Doxyfiles is to ensure that the documentation for each library is generated independently. This separation allows for more precise control over the documentation settings and configurations specific to each library. It also helps in maintaining clarity and organization, as the documentation requirements for libCZI and libCZI-API may differ.

Compiling Documentation for libCZI and libCZI-API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To compile the documentation for both libCZI and libCZI-API, follow these steps:


.. code-block:: bash
	
	# Navigate to the libCZI source directory
	cd Src/libCZI

	# Run Doxygen to generate the documentation
	doxygen


.. code-block:: bash

	# Navigate to the libCZIAPI source directory
	cd Src/libCZIAPI

	# Run Doxygen to generate the documentation
	doxygen

The doxygen command processes the Doxyfile configuration and scans the source code files to extract comments and documentation. It then generates the documentation in various formats, such as HTML (in our settings, Doxygen generates the documentation specifically as XML files), LaTeX, and XML, based on the settings specified in the Doxyfile.
Note: Doxygen automatically finds the Doxyfile, but remember that if you run only doxygen, the Doxyfile needs to be in the same directory.

Generating Documentation with Sphinx
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Once the documentation compilation is complete, you can generate the documentation using Sphinx. There are two methods to achieve this: using Python packages or a Docker container.

Method 1: Using Python Packages
-------------------------------
1. Install Sphinx and necessary extensions:

.. code-block:: bash

	pip install sphinx breathe exhale sphinx-book-theme recommonmark sphinx-markdown-tables

2. Install the extension for multiple Doxygen projects with Exhale:

.. code-block:: bash

	pip install -e "git+https://github.com/mithro/sphinx-contrib-mithro#egg=sphinx-contrib-exhale-multiproject&subdirectory=sphinx-contrib-exhale-multiproject"

Method 2: Using Docker Container
--------------------------------
1. Navigate to the Dockerfile:

.. code-block:: bash

	cd docs/scripts

2. Build a custom Docker image with the required dependencies:

.. code-block:: bash
	
	docker build -t custom-sphinx-image .

3. Run the Docker container to generate the documentation:

.. code-block:: bash
	
	docker run -it --rm -v "<host's repository root of libCZI>:/repo" custom-sphinx-image sphinx-build -M html repo/docs/source repo/docs/build

This command will create a self-contained documentation build on the host machine in the directory d:/repos/libCZI-C_API/docs/build.
