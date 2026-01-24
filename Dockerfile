FROM ubuntu:22.04

# インタラクティブな入力を防ぐ
ENV DEBIAN_FRONTEND=noninteractive

# 必要なパッケージのインストール
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libsfml-dev \
    libx11-dev \
    libxext-dev \
    libxrender-dev \
    libxinerama-dev \
    libxi-dev \
    libxrandr-dev \
    libxcursor-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# 作業ディレクトリの設定
WORKDIR /app

# コンパイルコマンド
CMD ["make"]


# FROM ubuntu:22.04
# ENV DEBIAN_FRONTEND=noninteractive

# RUN apt-get update && apt-get install -y \
#     build-essential \
#     libsfml-dev \
#     x11vnc \
#     xvfb \
#     fluxbox \
#     wget \
#     git \
#     python3 \
#     python3-pip \
#     && apt-get clean && rm -rf /var/lib/apt/lists/*

# RUN pip3 install websockify

# RUN wget -qO- https://github.com/novnc/noVNC/archive/v1.3.0.tar.gz | tar xz -C /opt \
#     && ln -s /opt/noVNC-1.3.0/vnc.html /opt/noVNC-1.3.0/index.html

# WORKDIR /app
# COPY start.sh /start.sh

# EXPOSE 8080
# CMD ["/start.sh"]
