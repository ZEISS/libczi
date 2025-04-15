# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys
sys.path.insert(0, os.path.abspath('.'))

import exhale_multiproject_monkeypatch

project = 'libCZI'
copyright = '2025, Zeiss Microscopy GmbH'
author = 'Zeiss Microscopy GmbH'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['breathe', 'exhale', 'recommonmark', 'sphinx_markdown_tables']

source_parsers = {
    '.md': 'recommonmark.parser.CommonMarkParser',
}

source_suffix = ['.rst', '.md']

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# html_theme = 'alabaster'
html_theme = 'sphinx_book_theme'
html_static_path = ['_static']


breathe_projects = {
    "Src" : "../../Src/Build/xml",
    "Api" : "../../Src/libCZIAPI/Build/xml"
}

breathe_default_project = "Src"


exhale_args = {
    'verboseBuild' : True,
    # These arguments are required
    "rootFileTitle":         "unknown",
    "containmentFolder":     "./lib",
    "rootFileName":          "index.rst",
    "doxygenStripFromPath":  "../../Src/",
    # Suggested optional arguments
    "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
}


exhale_projects_args = {
    "Src" : {
        "containmentFolder":     os.path.abspath("./lib"),
        "rootFileTitle":         "C++ Library",
    },
    "Api" : {
        "containmentFolder":     os.path.abspath("./api"),
        "rootFileTitle":         "C API",
    }
}
