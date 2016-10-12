/**
 * Author : Dayao Ji <jdy@rock-chips.com>
 *
 * Description : 
 */
#ifndef __REMOTEBUFFER_H__
#define __REMOTEBUFFER_H__


namespace rockchip {


class RemoteBufferWrapper {
public:
	RemoteBufferWrapper();
	~RemoteBufferWrapper();

	int connect(void);
	int disconnect(void);
	int removeClient(void);


private:
	
};


};

#endif /* __REMOTEBUFFER_H__ */
