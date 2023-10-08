FROM continuumio/miniconda3:latest

WORKDIR /stable-retro

COPY tests tests
COPY wheelhouse wheelhouse
CMD ["bash", "./tests/test_cibuildwheel/install_and_test_wheel.sh"]
