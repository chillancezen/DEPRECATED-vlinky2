VLINK_TOP_DIR=${VLINK_SDK}
INCLUDE_DIR=-I. 
LINK_DIR=-L. 

INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlinkinfra/include  
INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlink-backend 
INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlinkguest/include  
LINK_DIR+=-L$(VLINK_TOP_DIR)/vlinkinfra 
LINK_DIR+=-L$(VLINK_TOP_DIR)/vlinkguest 


