import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name='pitayaserver',
    version='0.3.0',
    author="TFG Co",
    author_email="backend@tfgco.com",
    description="A library for creating pitaya backend servers using python",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/topfreegames/libpitaya-cluster",
    packages=setuptools.find_packages(),
    install_requires=[
        'protobuf',
        'uuid',
    ],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
