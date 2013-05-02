MAKEAPPS := rfclient

# For now, fpm.h and fpm_lsp.h are included in the rfclient/ directory, however
# when these headers are standardised in Quagga upstream, we should include
# them directly from the Quagga source path.

# The below line enables the FPM connection to Quagga, the include directory
# needs to point to fpm.h in quagga
# CFLAGS += -DFPM_ENABLED -I../../quagga-fpm/fpm/

include ../Make.rules
