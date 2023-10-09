FROM continuumio/miniconda3:latest

WORKDIR /stable-retro

RUN apt update && apt install -y freeglut3-dev xvfb

COPY tests tests
COPY wheelhouse wheelhouse
CMD ["bash", "./tests/test_cibuildwheel/install_and_test_wheel.sh"]
