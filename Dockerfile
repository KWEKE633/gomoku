# FROM ubuntu:22.04

# # インタラクティブな入力を防ぐ
# ENV DEBIAN_FRONTEND=noninteractive

# # 必要なパッケージのインストール
# RUN apt-get update && apt-get install -y \
#     build-essential \
#     cmake \
#     libsfml-dev \
#     libx11-dev \
#     libxext-dev \
#     libxrender-dev \
#     libxinerama-dev \
#     libxi-dev \
#     libxrandr-dev \
#     libxcursor-dev \
#     libgl1-mesa-dev \
#     libglu1-mesa-dev \
#     g++ \
#     make \
#     && rm -rf /var/lib/apt/lists/*

# # 作業ディレクトリの設定
# WORKDIR /app

# # コンパイルコマンド
# CMD ["make"]


FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 必要なパッケージ（SFML + 仮想デスクトップ + noVNC + Python3）
RUN apt-get update && apt-get install -y \
    build-essential \
    libsfml-dev \
    x11vnc \
    xvfb \
    fluxbox \
    wget \
    git \
    python3 \
    python3-pip \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# websockify（ブラウザ通信用）のインストール
RUN pip3 install websockify

# noVNCのインストール
RUN wget -qO- https://github.com/novnc/noVNC/archive/v1.3.0.tar.gz | tar xz -C /opt \
    && ln -s /opt/noVNC-1.3.0/vnc.html /opt/noVNC-1.3.0/index.html

WORKDIR /app

# ポート8080を開放
EXPOSE 8080

# 仮想ディスプレイを起動し、VNCをブラウザ経由で中継
# DISPLAY=:1 で仮想画面を定義します
CMD Xvfb :1 -screen 0 1024x768x24 & \
    sleep 2 && \
    DISPLAY=:1 fluxbox & \
    x11vnc -display :1 -nopw -forever -listen localhost & \
    /opt/noVNC-1.3.0/utils/novnc_proxy --vnc localhost:5900 --listen 8080