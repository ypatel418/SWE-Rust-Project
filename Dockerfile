# docker build --platform linux/amd64 -t image-conversion-server:latest . 
# docker run --platform linux/amd64 -p 8080:8080 image-conversion-server:latest
# ====================================================================
# STAGE 1: builder-x86 (For macOS/ARM Host)
# Uses Stable-Bookworm (Debian 12) for glibc compatibility
# ====================================================================
FROM rust:bookworm AS builder 

# Set the main working directory for the workspace root
WORKDIR /app

# Install the x86_64 target toolchain
RUN rustup target add x86_64-unknown-linux-gnu

# Install C FFI dependencies from Bookworm repos
RUN apt-get update && apt-get install -y \
    pkg-config \
    gcc \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copy the entire project into the container's /app folder
COPY . .

# Create a compatibility shim for the missing glibc symbols
RUN echo '#include <stdlib.h>' > /tmp/glibc_compat.c && \
    echo '#include <stdarg.h>' >> /tmp/glibc_compat.c && \
    echo '#include <stdio.h>' >> /tmp/glibc_compat.c && \
    echo '' >> /tmp/glibc_compat.c && \
    echo 'long __isoc23_strtol(const char *nptr, char **endptr, int base) {' >> /tmp/glibc_compat.c && \
    echo '    return strtol(nptr, endptr, base);' >> /tmp/glibc_compat.c && \
    echo '}' >> /tmp/glibc_compat.c && \
    echo '' >> /tmp/glibc_compat.c && \
    echo 'unsigned long __isoc23_strtoul(const char *nptr, char **endptr, int base) {' >> /tmp/glibc_compat.c && \
    echo '    return strtoul(nptr, endptr, base);' >> /tmp/glibc_compat.c && \
    echo '}' >> /tmp/glibc_compat.c && \
    echo '' >> /tmp/glibc_compat.c && \
    echo 'int __isoc23_vsscanf(const char *str, const char *format, va_list ap) {' >> /tmp/glibc_compat.c && \
    echo '    return vsscanf(str, format, ap);' >> /tmp/glibc_compat.c && \
    echo '}' >> /tmp/glibc_compat.c && \
    echo '' >> /tmp/glibc_compat.c && \
    echo 'int __isoc23_sscanf(const char *str, const char *format, ...) {' >> /tmp/glibc_compat.c && \
    echo '    va_list ap;' >> /tmp/glibc_compat.c && \
    echo '    va_start(ap, format);' >> /tmp/glibc_compat.c && \
    echo '    int result = vsscanf(str, format, ap);' >> /tmp/glibc_compat.c && \
    echo '    va_end(ap);' >> /tmp/glibc_compat.c && \
    echo '    return result;' >> /tmp/glibc_compat.c && \
    echo '}' >> /tmp/glibc_compat.c

# Compile the compatibility shim into a static library
RUN gcc -c /tmp/glibc_compat.c -o /tmp/glibc_compat.o && \
    ar rcs /tmp/libglibc_compat.a /tmp/glibc_compat.o

# Set up the linker to use our compatibility library
ENV RUSTFLAGS="-C link-arg=/tmp/libglibc_compat.a"
ENV CFLAGS="-std=c11"

# CRITICAL FIX: Compile the server executable, explicitly targeting x86_64
RUN cargo build --release --target x86_64-unknown-linux-gnu --manifest-path RustFFI/server/Cargo.toml

# ====================================================================
# STAGE 2: runtime
# Uses Bookworm-slim to provide a modern, minimal base with compatible glibc.
# ====================================================================
FROM debian:bookworm-slim AS runtime

WORKDIR /app

EXPOSE 8080

# Copy the final server executable from the builder stage
COPY --from=builder /app/RustFFI/server/target/x86_64-unknown-linux-gnu/release/server /usr/local/bin/server

# Copy the static directory from the correct location
COPY --from=builder /app/RustFFI/server/static ./static

# Copy the Rocket.toml configuration
COPY --from=builder /app/RustFFI/server/Rocket.toml ./Rocket.toml

# Set the entrypoint to run your application
CMD ["/usr/local/bin/server"]