# FROM debian:bullseye-slim
ARG DOCKER_TAG=v5.4.1
FROM espressif/idf:${DOCKER_TAG}
ENV DEBIAN_FRONTEND=noninteractive
# ENV LC_ALL=C.UTF-8
# ENV LANG=C.UTF-8
ENV IDF_CCACHE_ENABLE=1
# ARG CONTAINER_USER=esp
# ARG CONTAINER_GROUP=esp




ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN apt-get update -y && apt-get install udev -y

RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]

CMD ["/bin/bash", "-c"]

# # libpython2.7 is due to GDB
# RUN apt-get update \
#     && apt-get install -y git curl wget flex bison gperf python3 python3-venv python3-pip  \
#     python3-setuptools ninja-build ccache libffi-dev libssl-dev dfu-util \
#     libusb-1.0-0 libpython2.7 unzip \
#     && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts
# RUN adduser --disabled-password --gecos "" ${CONTAINER_USER}
# USER ${CONTAINER_USER}
# ENV USER=${CONTAINER_USER}
# WORKDIR /home/${CONTAINER_USER}
# RUN mkdir -p .espressif/frameworks/ \
#     && git clone --branch ${ESP_IDF_VERSION} -q --depth 1 --shallow-submodules \
#     --recursive https://github.com/espressif/esp-idf.git \
#     .espressif/frameworks/esp-idf \
#     && python3 .espressif/frameworks/esp-idf/tools/idf_tools.py install cmake \
#     && .espressif/frameworks/esp-idf/install.sh ${ESP_BOARD}
# RUN curl -L https://github.com/bjoernQ/esp-web-flash-server/releases/latest/download/web-flash-x86_64-unknown-linux-gnu.zip \
#     -o /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/web-flash.zip \
#     && unzip /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/web-flash.zip \
#     -d /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/ \
#     && rm /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/web-flash.zip
# RUN chmod u+x /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/web-flash
# # RUN curl -L https://github.com/MabezDev/wokwi-server/releases/latest/download/wokwi-server-x86_64-unknown-linux-gnu.zip \
# #     -o /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/wokwi-server.zip \
# #     && unzip /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/wokwi-server.zip \
# #     -d /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/ \
# #     && rm /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/wokwi-server.zip
# # RUN chmod u+x /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/tools/wokwi-server
# ENV IDF_TOOLS_PATH=/home/${CONTAINER_USER}/.espressif
# RUN echo "source /home/${CONTAINER_USER}/.espressif/frameworks/esp-idf/export.sh > /dev/null 2>&1" >> ~/.bashrc
# CMD "/bin/bash"
