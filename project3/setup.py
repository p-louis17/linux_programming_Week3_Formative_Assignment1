from setuptools import setup, Extension

setup(
    name="faststats",
    version="1.0",
    ext_modules=[
        Extension(
            "faststats",
            sources=["fastlib/faststats.c"],
            extra_compile_args=["-O2", "-Wall", "-Wextra"],
        )
    ],
)