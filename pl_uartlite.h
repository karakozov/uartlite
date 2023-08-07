
#ifndef PL_UART_LITE_H
#define PL_UART_LITE_H

#include "mapper.h"
#include "time_ipc.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>

//-----------------------------------------------------------------------------

using job_t = std::shared_ptr<std::thread>;

//-----------------------------------------------------------------------------

template <typename job_type, typename... args_type>
auto make_job(args_type &&...args)
{
    return std::make_shared<job_type>(std::forward<args_type>(args)...);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

namespace pl_uartlite
{
    enum uatlite_registers
    {
        UART_RX_FIFO = 0x0,
        UART_TX_FIFO = 0x4,
        UART_STATUS = 0x8,
        UART_CTRL = 0xC,
    };

    struct uatlite_rx_fifo_bitmask
    {
        uint32_t
            RX_DATA : 8,
            : 24; ///< Резерв.
    };

    struct uatlite_tx_fifo_bitmask
    {
        uint32_t
            RX_DATA : 8,
            : 24; ///< Резерв.
    };

    struct uatlite_control_bitmask
    {
        uint32_t
            RST_TX_FIFO : 1,
            RST_RX_FIFO : 1, : 2,
            ENABLE_INTR : 1,
            : 27; ///< Резерв.
    };

    struct uatlite_status_bitmask
    {
        uint32_t
            RX_FIFO_VALID_DATA : 1,
            RX_FIFO_FULL : 1,
            TX_FIFO_EMPTY : 1,
            TX_FIFO_FULL : 1,
            INTR_ENAABLED : 1,
            OVERRUN_ERROR : 1,
            FRAME_ERROR : 1,
            PARITY_ERROR : 1,
            : 24; ///< Резерв.
    };

    template <typename bitmask_type, typename reg_type>
    union data_type
    {
        reg_type value;
        bitmask_type bits;
    };

    using reg_rx_fifo = data_type<uatlite_rx_fifo_bitmask, uint32_t>;
    using reg_tx_fifo = data_type<uatlite_tx_fifo_bitmask, uint32_t>;
    using reg_ctrl = data_type<uatlite_control_bitmask, uint32_t>;
    using reg_status = data_type<uatlite_status_bitmask, uint32_t>;

    class pl_uart
    {
    public:
        pl_uart(uint32_t base_address,
                uint32_t size,
                std::deque<uint8_t> &rd_queue,
                std::mutex &rd_lock,
                std::deque<uint8_t> &wr_queue,
                std::mutex &wr_lock) : read_queue(rd_queue), read_lock(rd_lock), write_queue(wr_queue), write_lock(wr_lock)
        {
            _base = nullptr;
            _mapper = get_mapper<Mapper>();
            if (_mapper.get())
            {
                _base = static_cast<uint32_t *>(_mapper->map(base_address, size));
            }

            rx_fifo_reg = (reg_rx_fifo *)&_base[UART_RX_FIFO >> 2];
            tx_fifo_reg = (reg_tx_fifo *)&_base[UART_TX_FIFO >> 2];
            ctrl_reg = (reg_ctrl *)&_base[UART_CTRL >> 2];
            status_reg = (reg_status *)&_base[UART_STATUS >> 2];

            ctrl_reg->value = 0;

            fprintf(stderr, "UART_CTRL = 0x%x\n", ctrl_reg->value);
            fprintf(stderr, "UART_STAT = 0x%x\n", status_reg->value);
        }

        virtual ~pl_uart()
        {
            stop();
            ipc_delay(100);
            _base = nullptr;
        }

        ssize_t read_thread()
        {
            ctrl_reg->bits.ENABLE_INTR = 0;
            ctrl_reg->bits.RST_RX_FIFO = 1;

            ssize_t readed = 0;

            fprintf(stderr, "%s(): UART_CTRL = 0x%x\n", __func__, ctrl_reg->value);
            fprintf(stderr, "%s(): UART_STAT = 0x%x\n", __func__, status_reg->value);            

            while (!is_exit)
            {
                if(status_reg->bits.RX_FIFO_VALID_DATA)
                {
                    if (is_exit)
                        return readed;

                    std::lock_guard<std::mutex> _lock(write_lock);
                    uint8_t v = rx_fifo_reg->value;
                    read_queue.push_back(v);
                    ++readed;

                } else {
                    ipc_delay(5);
                    if (is_exit)
                        return readed;                    
                }
            }

            fprintf(stderr, "OK: readed %ld bytes\n", readed);

            return readed;
        };

        ssize_t write_thread()
        {
            ctrl_reg->bits.ENABLE_INTR = 0;
            ctrl_reg->bits.RST_TX_FIFO = 1;

            ssize_t written = 0;

            fprintf(stderr, "%s(): UART_CTRL = 0x%x\n", __func__, ctrl_reg->value);
            fprintf(stderr, "%s(): UART_STAT = 0x%x\n", __func__, status_reg->value);            

            while (!is_exit)
            {
                //fprintf(stderr, "%d: %s()\n", __LINE__, __func__);

                while (status_reg->bits.TX_FIFO_FULL)
                {
                    ipc_delay(5);
                    if (is_exit)
                        return written;
                }

                {
                    //fprintf(stderr, "%d: %s()\n", __LINE__, __func__);

                    if (write_queue.empty())
                    {
                        ipc_delay(5);
                        continue;
                    }

                    while (!status_reg->bits.TX_FIFO_FULL)
                    {
                        //fprintf(stderr, "%d: %s()\n", __LINE__, __func__);
                        
                        if (is_exit)
                            return written;

                        if (!write_queue.empty())
                        {
                            std::lock_guard<std::mutex> _lock(write_lock);
                            uint8_t v = write_queue.front();
                            tx_fifo_reg->value = v;
                            ++written;
                            write_queue.pop_front();
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }

            fprintf(stderr, "OK: written %ld bytes\n", written);

            return written;
        };

        void stop()
        {
            is_exit = true;
        }

    private:
        mapper_t _mapper;
        uint32_t *_base;
        reg_rx_fifo *rx_fifo_reg = {nullptr};
        reg_tx_fifo *tx_fifo_reg = {nullptr};
        reg_ctrl *ctrl_reg = {nullptr};
        reg_status *status_reg = {nullptr};
        std::deque<uint8_t> &read_queue;
        std::deque<uint8_t> &write_queue;
        std::mutex &read_lock;
        std::mutex &write_lock;
        std::vector<job_t> jobs;
        bool is_exit{false};
    };
};

    //------------------------------------------------------------------------------

#endif // PL_UART_LITE_H
