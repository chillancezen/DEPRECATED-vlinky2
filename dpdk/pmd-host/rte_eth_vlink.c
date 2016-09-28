/*2nd vlink which decouplefunctions between dpdk and qemu via standalone agent*/

#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_memcpy.h>
#include <rte_dev.h>
#include <rte_kvargs.h>
#include <virtbus_client.h>
#include <agent.h>


#define VLINK_BURST_RXTX_SIZE 64 

#define ETH_VLINK_ARG_LINK "link"
#define TRE_PMD_VLINK_DBG
static const char*driver_name="virtual link dpdk-lane PMD";


#if defined(TRE_PMD_VLINK_DBG)
	#define VLINK_LOG(format, ...)  fprintf (stdout, format, __VA_ARGS__)
#else 
	#define VLINK_LOG(format, ...) {}
#endif


struct vlink_data_channel{
	int channel_id;/*local channel index*/
	struct queue_stub *rx_stub;
	struct queue_stub *free_stub;
	struct queue_stub *alloc_stub;
	struct queue_stub *tx_stub;
	struct rte_mempool *mp_buff;
	struct rte_eth_dev *dev;/*reference back*/
};
struct vlink_pmd_private{
	struct client_endpoint *ce;
	uint64_t shm_base;
	struct ctrl_channel *ctrl;/*reference to ctrl channel in shm*/
	struct vlink_data_channel rxtx_data_channels[RTE_MAX_QUEUES_PER_PORT];
};

static int vlink_dev_start(struct rte_eth_dev*dev)
{
	dev->data->dev_link.link_status=ETH_LINK_UP;
	return 0;
	
}
static void vlink_dev_stop(struct rte_eth_dev*dev)
{
	dev->data->dev_link.link_status=ETH_LINK_DOWN;
}
static int vlink_dev_configure(struct rte_eth_dev *dev __rte_unused)
{
	return 0;
}
static void vlink_dev_info(struct rte_eth_dev* dev,struct rte_eth_dev_info *dev_info)
{
	struct vlink_pmd_private *private=dev->data->dev_private;
	dev_info->driver_name=driver_name;
	dev_info->max_mac_addrs=1;
	dev_info->max_rx_pktlen=(uint32_t)-1;
	dev_info->max_rx_queues=private->ce->allocated_channels-1;
	dev_info->max_tx_queues=private->ce->allocated_channels-1;
	dev_info->min_rx_bufsize=0;
	dev_info->pci_dev=NULL;
}
static int vlink_dev_rx_queue_setup(struct rte_eth_dev* dev,
	uint16_t rx_queue_id,
	uint16_t nb_rx_desc __rte_unused,
	unsigned int socket_id __rte_unused,
	const struct rte_eth_rxconf *rx_conf __rte_unused,
	struct rte_mempool*mb_pool)
{
	struct vlink_pmd_private*private=dev->data->dev_private;
	if(rx_queue_id>=private->ctrl->nr_data_channels)
		return -1;
	private->rxtx_data_channels[rx_queue_id].mp_buff=mb_pool;
	private->ctrl->channel_records[rx_queue_id].data_channel_enabled=1;
	
	dev->data->rx_queues[rx_queue_id]=&(private->rxtx_data_channels[rx_queue_id]);
	VLINK_LOG("\n[x]setup %s rx queue:%d(%d)\n",private->ce->vlink_name,rx_queue_id,private->ctrl->nr_data_channels);
	return  0;
}
static int vlink_dev_tx_queue_setup(struct rte_eth_dev*dev,
	uint16_t tx_queue_id,
	uint16_t nb_tx_desc __rte_unused,
	unsigned int socket_id __rte_unused,
	const struct rte_eth_txconf*tx_conf __rte_unused)
{
	struct vlink_pmd_private*private=dev->data->dev_private;
	if(tx_queue_id>=private->ctrl->nr_data_channels)
		return -1;
	dev->data->tx_queues[tx_queue_id]=&(private->rxtx_data_channels[tx_queue_id]);
	VLINK_LOG("[x]setup %s tx queue:%d(%d)\n",private->ce->vlink_name,tx_queue_id,private->ctrl->nr_data_channels);
	return 0;
}
static void vlink_dev_queue_release(void*q __rte_unused)
{
	
}

static int vlink_dev_link_update(struct rte_eth_dev *dev __rte_unused,int wait_to_complete __rte_unused ) 
{
	return 0; 
}

static int argument_callback_link(const char* key __rte_unused,const char*value,void*extra)
{
	sprintf(extra,"%s%c",value,'\0');
	return 0;
}
static const char*valid_arguments[]={
	ETH_VLINK_ARG_LINK,
	NULL,
};
static struct rte_eth_link pmd_link={
	.link_speed=ETH_SPEED_NUM_10G,
	.link_duplex=ETH_LINK_FULL_DUPLEX,
	.link_status=ETH_LINK_DOWN,
	.link_autoneg=ETH_LINK_SPEED_AUTONEG,
};


static struct eth_dev_ops dev_ops={
	.dev_start=vlink_dev_start,
	.dev_stop=vlink_dev_stop,
	.dev_configure=vlink_dev_configure,
	.dev_infos_get=vlink_dev_info,
	.rx_queue_setup=vlink_dev_rx_queue_setup,
	.tx_queue_setup=vlink_dev_tx_queue_setup,
	.rx_queue_release=vlink_dev_queue_release,
	.tx_queue_release=vlink_dev_queue_release,
	.link_update=vlink_dev_link_update,
	
};

static uint16_t vlink_rx(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
	struct queue_element qele[VLINK_BURST_RXTX_SIZE];
	struct vlink_data_channel * channel=PTR(struct vlink_data_channel*,q);
	struct rte_eth_dev * dev=channel->dev;
	struct vlink_pmd_private*private=dev->data->dev_private;
	int ready_to_recv=MIN(VLINK_BURST_RXTX_SIZE,nb_bufs);
	int nr_dequeued;
	int idx;
	/*1.receive as much packets as possible from rx sub-channel*/
	nr_dequeued=dequeue_bulk(channel->rx_stub,qele,ready_to_recv);
	for(idx=0;idx<nr_dequeued;idx++){//translation physical address back to virtual address
		
	}
	return 0;
}
static uint16_t vlink_tx(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs)
{
	struct queue_element qele[VLINK_BURST_RXTX_SIZE];
	struct vlink_data_channel * channel=PTR(struct vlink_data_channel*,q);
	struct rte_eth_dev * dev=channel->dev;
	struct vlink_pmd_private*private=dev->data->dev_private;
	int idx=0;
	int ready_to_xmit=MIN(nb_bufs,VLINK_BURST_RXTX_SIZE);
	int nr_enqueued;
	for(idx=0;idx<ready_to_xmit;idx++){
		qele[idx].rte_pkt_offset=translate_virt_address(PTR(uint64_t,bufs[idx]));
		qele[idx].rte_data_offset=translate_virt_address(rte_pktmbuf_mtod(bufs[idx],uint64_t));
		/*RTE_ASSERT(PTR(uint64_t,bufs[idx])==translate_phy_address(translate_virt_address(PTR(uint64_t,bufs[idx]))));*/
	}
	nr_enqueued=enqueue_bulk(channel->tx_stub,qele,ready_to_xmit);
	
	printf("[x]%d %d %d\n",nr_enqueued,ready_to_xmit,translate_phy_address(0x1234567));
	
	
	return nr_enqueued;
}


static int rte_pmd_vlink_dev_init(const char*name,const char* params)
{
	char link_name[64];
	struct client_endpoint * ce=NULL;
	uint64_t vm_shm_base=0;
	struct rte_eth_dev *eth_dev=NULL;
	struct rte_eth_dev_data * data=NULL;
	struct vlink_pmd_private *private=NULL;
	
	struct rte_kvargs *kvlist;
	int ret=0;
	kvlist=rte_kvargs_parse(params,valid_arguments);
	if(!kvlist)
		return -1;
	ret=rte_kvargs_process(kvlist,ETH_VLINK_ARG_LINK,argument_callback_link,link_name);
	if(ret<0)
		goto kvlist_free;
	/*1.try to allocate vlink resource from agent*/
	ce=client_endpoint_alloc_and_init();
	if(!ce||(ce->is_connected==0))
		goto fails;
	ret=client_endpoint_request_virtual_link(ce,link_name,VLINK_ROLE_DPDK);
	if(ret)
		goto fails;
	
	
	VLINK_LOG("[x]vm_domain name:%s\n",ce->vm_domain);
	VLINK_LOG("[x]max channel in vm:%d\n",ce->max_channels);
	VLINK_LOG("[x]vm_link name:%s\n",ce->vlink_name);
	VLINK_LOG("[x]allocated channel:%d\n",ce->allocated_channels);
	VLINK_LOG("[x]allocated mac %02x:%02x:%02x:%02x:%02x:%02x\n",ce->mac_address[0],
		ce->mac_address[1],
		ce->mac_address[2],
		ce->mac_address[3],
		ce->mac_address[4],
		ce->mac_address[5]);
	{
		int idx;
		for(idx=0;idx<ce->allocated_channels;idx++){
			if(idx==0)
				VLINK_LOG("[x]channels:%d(ctrl),",ce->channels[idx]);
			else
				VLINK_LOG("%d,",ce->channels[idx]);
		}
		VLINK_LOG("%s\n","");
		
	}
	if(ce->allocated_channels<2)
		goto fails;
	
	/*2.map vm_domain shm*/
	vm_shm_base=map_shared_memory(ce->vm_domain,ce->max_channels*CHANNEL_SIZE);
	VLINK_LOG("[x]vm domain_base for %s:0x%"PRIx64"\n",ce->vm_domain,vm_shm_base);
	if(!vm_shm_base)
		goto fails;

	/*3.allocate pmd related dev */
	if(!(eth_dev=rte_eth_dev_allocate(name,RTE_ETH_DEV_VIRTUAL)))
		goto fails;
	if(!(data=rte_zmalloc(name,sizeof(struct rte_eth_dev_data),0)))
		goto fails;
	if(!(private=rte_zmalloc(name,sizeof(struct vlink_pmd_private),0)))
		goto fails;
	private->ce=ce;
	private->shm_base=vm_shm_base;
	private->ctrl=PTR_OFFSET_BY(struct ctrl_channel*,private->shm_base,channel_base_offset(ce->channels[0]));
	{
		ASSERT(((int)private->ctrl->nr_data_channels==(ce->allocated_channels-1)));
		int idx=0;
		for(;idx<(int)private->ctrl->nr_data_channels;idx++){
			private->rxtx_data_channels[idx].dev=eth_dev;
			private->rxtx_data_channels[idx].channel_id=idx;
			private->rxtx_data_channels[idx].mp_buff=NULL;
			private->rxtx_data_channels[idx].rx_stub=PTR_OFFSET_BY(struct queue_stub*,private->shm_base,private->ctrl->channel_records[idx].offset_to_vm_shm_base+rx_sub_channel_offset());
			private->rxtx_data_channels[idx].free_stub=PTR_OFFSET_BY(struct queue_stub*,private->shm_base,(private->ctrl->channel_records[idx].offset_to_vm_shm_base+free_sub_channel_offset()));
			private->rxtx_data_channels[idx].alloc_stub=PTR_OFFSET_BY(struct queue_stub*,private->shm_base,(private->ctrl->channel_records[idx].offset_to_vm_shm_base+alloc_sub_channel_offset()));
			private->rxtx_data_channels[idx].tx_stub=PTR_OFFSET_BY(struct queue_stub*,private->shm_base,(private->ctrl->channel_records[idx].offset_to_vm_shm_base+tx_sub_channel_offset()));
			private->ctrl->channel_records[idx].data_channel_enabled=0x0;
			private->ctrl->channel_records[idx].interrupt_enabled=0x0;
		}
	}
	
	data->dev_private=private;
	data->port_id=eth_dev->data->port_id;
	data->nb_rx_queues=ce->allocated_channels-1;
	data->nb_tx_queues=ce->allocated_channels-1;
	data->dev_link=pmd_link;
	data->mac_addrs=(struct ether_addr*)ce->mac_address;
	data->dev_flags=RTE_ETH_DEV_DETACHABLE;
	data->kdrv=RTE_KDRV_NONE;
	data->drv_name=driver_name;
	data->numa_node=rte_socket_id();
	strncpy(data->name, eth_dev->data->name, strlen(eth_dev->data->name));

	eth_dev->data=data;
	eth_dev->dev_ops=&dev_ops;
	eth_dev->driver=NULL;
	TAILQ_INIT(&eth_dev->link_intr_cbs);

	eth_dev->rx_pkt_burst=vlink_rx;
	eth_dev->tx_pkt_burst=vlink_tx;
	
	
	
	

	kvlist_free:
		if(kvlist)
			rte_kvargs_free(kvlist);
		return ret;
	fails:
		if(ce&&vm_shm_base)
			unmap_shared_memory(vm_shm_base,ce->max_channels*CHANNEL_SIZE);//assert ce is available
		if(ce)
			client_endpoint_uninit_and_dealloc(ce);
		if(private)
			rte_free(private);
		if(data)
			rte_free(data);
		#if 0
		if(eth_dev)
			rte_eth_dev_release_port(eth_dev);
		/*do not release port here,it's supposed that rte_pmd_vlink_dev_uninit will be invoked whether link init succeeds or not,
		but need verification*/
		#endif
		ret=-1;
		goto kvlist_free;
		

	
}
static int rte_pmd_vlink_dev_uninit(const char*name)
{
	struct rte_eth_dev  *eth_dev=NULL;
	struct vlink_pmd_private*private=NULL;
	eth_dev=rte_eth_dev_allocated(name);
	if(!eth_dev)
		return -1;
	/*1.free anythig in client-endpoint*/
	private=((struct vlink_pmd_private*)(eth_dev->data->dev_private));
	if(private->ce&&private->shm_base)
		unmap_shared_memory(private->shm_base,private->ce->allocated_channels*CHANNEL_SIZE);
	if(private->ce)
		client_endpoint_uninit_and_dealloc(private->ce);

	/*2.release private,data,end ports*/
	if(eth_dev->data&&eth_dev->data->dev_private)
		rte_free(eth_dev->data->dev_private);
	if(eth_dev->data)
		rte_free(eth_dev->data);
	rte_eth_dev_release_port(eth_dev);
	

	return 0;
}

static struct rte_driver pmd_vlink_drv={
	.type=PMD_VDEV,
	.init=rte_pmd_vlink_dev_init,
	.uninit=rte_pmd_vlink_dev_uninit,
};
PMD_REGISTER_DRIVER(pmd_vlink_drv,eth_vlink);
DRIVER_REGISTER_PARAM_STRING(eth_vlink,
	"link=<string>");



