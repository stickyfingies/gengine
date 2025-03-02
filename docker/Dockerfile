####  ####  ####
# Docker/Cmake   https://www.jmoisio.eu/en/blog/2020/06/01/building-cpp-containers-using-docker-and-cmake/
# Docker/Ubuntu  https://octopus.com/blog/using-ubuntu-docker-image
####  ####  ####

FROM ubuntu:latest

# Create the /devops directory and set permissions
WORKDIR /
COPY . /docker/
RUN chmod -R a+rw /docker

# Install system packages from 'packages.txt'
RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker
RUN apt-get update
RUN apt-get install -y $(cat /docker/packages.txt)
RUN rm -rf /var/lib/apt/lists/*

# Set these as volume mount points
VOLUME /dependencies
VOLUME /devops
VOLUME /source

# Run the setup script
CMD ["bash"]