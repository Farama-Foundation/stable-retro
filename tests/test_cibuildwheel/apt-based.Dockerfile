FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/London

WORKDIR /stable-retro

# Install Python and pip, OpenGL, and Xvfb
RUN apt update && apt install -y python3-dev python3-pip python3-opengl freeglut3-dev xvfb

COPY tests tests
COPY wheelhouse wheelhouse
CMD ["bash", "./tests/test_cibuildwheel/install_and_test_wheel.sh"]
