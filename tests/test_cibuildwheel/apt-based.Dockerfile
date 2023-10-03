FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/London

WORKDIR /stable-retro

# Install Python and pip
RUN apt update && apt install -y python3-dev python3-pip

COPY . ./
CMD ["bash", "./tests/test_cibuildwheel/install_and_test_wheel.sh"]
