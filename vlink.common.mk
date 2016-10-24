VLINK_TOP_DIR=/root/mywork/vlink2.new
INCLUDE_DIR=-I. 
LINK_DIR=-L. 

INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlinkinfra/include  
INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlink-frontend 
INCLUDE_DIR+=-I$(VLINK_TOP_DIR)/vlinkguest/include  
LINK_DIR+=-L$(VLINK_TOP_DIR)/vlinkinfra 
LINK_DIR+=-L$(VLINK_TOP_DIR)/vlinkguest 


