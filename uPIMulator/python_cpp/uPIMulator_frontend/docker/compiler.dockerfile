FROM bongjoonhyun/compiler-base:latest

# PIM-CCA LLVM Pass
ENV CMAKE_PREFIX_PATH="/root/install/upmem-llvm"
ENV PATH="$PATH:/root/install/upmem-llvm/bin"
WORKDIR /root
RUN git clone https://github.com/SeokWonKang/cca-llvm-pass.git
RUN mkdir -p /root/cca-llvm-pass/build
WORKDIR /root/cca-llvm-pass/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/root/install/cca-llvm-pass /root/cca-llvm-pass
RUN make
RUN make install

# Alias
WORKDIR /root
RUN mkdir -p /root/bin-wrapper
RUN echo '#!/bin/bash' > /root/bin-wrapper/dpu-upmem-dpurte-clang
RUN echo '/root/upmem-2021.3.0-Linux-x86_64/bin/dpu-upmem-dpurte-clang -fexperimental-new-pass-manager -fpass-plugin=/root/install/cca-llvm-pass/lib/libPIMCCALLVMInstrumentation.so $@' >> /root/bin-wrapper/dpu-upmem-dpurte-clang
RUN echo 'bash /root/install/cca-llvm-pass/bin/RemoveComment.sh $@' >> /root/bin-wrapper/dpu-upmem-dpurte-clang
RUN chmod a+x /root/bin-wrapper/dpu-upmem-dpurte-clang

RUN echo '#!/bin/bash' > /root/bin-wrapper/dpu-clang
RUN echo '/root/upmem-2021.3.0-Linux-x86_64/bin/dpu-clang -fexperimental-new-pass-manager -fpass-plugin=/root/install/cca-llvm-pass/lib/libPIMCCALLVMInstrumentation.so $@' >> /root/bin-wrapper/dpu-clang
RUN echo 'bash /root/install/cca-llvm-pass/bin/RemoveComment.sh $@' >> /root/bin-wrapper/dpu-clang
RUN chmod a+x /root/bin-wrapper/dpu-clang

RUN echo '#!/bin/bash' > /root/bin-wrapper.sh
RUN echo 'export PATH="/root/bin-wrapper:${PATH}"' >> /root/bin-wrapper.sh

WORKDIR /root/upmem_compiler
