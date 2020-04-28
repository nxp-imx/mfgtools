#include "hidreport.h"
#include "liberror.h"

void HIDReport::notify(size_t index, uuu_notify::NOTIFY_TYPE type)
{
	uuu_notify nf;
	nf.type = type;
	if(type == uuu_notify::NOTIFY_TRANS_POS)
		nf.index = index + m_postion_base;
	if (type == uuu_notify::NOTIFY_TRANS_SIZE)
	{
		nf.index = m_notify_total > index ? m_notify_total : index;
	}
	call_notify(nf);
}

int HIDReport::read(vector<uint8_t> &buff)
{
	size_t rs;
	if (buff.size() < m_size_in + m_size_payload)
	{
		set_last_err_string("buffer to small to get a package");
		return -1;
	}
	int ret = m_pdev->read(buff.data(), m_size_in + m_size_payload, &rs);
	if (ret)
		return ret;

	return ret;
}

int HIDReport::write(void *p, size_t sz, uint8_t report_id)
{
	size_t off;
	uint8_t *buff = (uint8_t *)p;

	notify(sz, uuu_notify::NOTIFY_TRANS_SIZE);

	for (off = 0; off < sz; off += m_size_out)
	{
		m_out_buff[0] = report_id;

		size_t s = sz - off;
		if (s > m_size_out)
			s = m_size_out;

		memcpy(m_out_buff.data() + m_size_payload, buff + off, s);

		int ret = m_pdev->write(m_out_buff.data(), report_id == 1? s + m_size_payload: m_size_out + m_size_payload);

		if ( ret < 0)
			return -1;

		if (off % 0x1F == 0)
		{
			notify(off, uuu_notify::NOTIFY_TRANS_POS);
		}
	}
	notify(off, uuu_notify::NOTIFY_TRANS_POS);
	return 0;
}
