FROM python:3.12-bookworm

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        libsdl2-dev \
        novnc \
        websockify \
        x11vnc \
        xvfb \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --no-cache-dir platformio

WORKDIR /app
COPY . .
RUN pio run -e display-sim

EXPOSE 6080
ENV DISPLAY=:99

CMD ["bash", "-lc", "Xvfb :99 -screen 0 520x520x24 -nolisten tcp & x11vnc -display :99 -forever -shared -nopw -rfbport 5900 >/tmp/x11vnc.log 2>&1 & websockify --web=/usr/share/novnc/ 6080 localhost:5900 >/tmp/novnc.log 2>&1 & exec ./.pio/build/display-sim/program"]
