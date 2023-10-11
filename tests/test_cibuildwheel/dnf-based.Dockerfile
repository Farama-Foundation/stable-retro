FROM fedora:latest

WORKDIR /stable-retro

# Install Python and pip, OpenGL, and Xvfb
RUN dnf update -y && dnf clean all && dnf install -y python3-devel python3-pip freeglut-devel xorg-x11-server-Xvfb

COPY tests tests
COPY wheelhouse wheelhouse
CMD ["bash", "./tests/test_cibuildwheel/install_and_test_wheel.sh"]
