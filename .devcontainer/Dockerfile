FROM danwos/sphinxneeds:latest

USER root

RUN apt-get update && apt-get install -y \
    locales \
    git \
    curl \
    wget \
    unzip \
    zip \
    build-essential \
    ca-certificates \
    vim \
    nano \
    htop && \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    locale-gen \
    && rm -rf /var/lib/apt/lists/*
RUN pip install sphinx-rtd-theme
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8