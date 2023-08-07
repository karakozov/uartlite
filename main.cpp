
#include "config_parser.h"
#include "pl_uartlite.h"

//-----------------------------------------------------------------------------

#include <cstdint>
#include <csignal>

//-----------------------------------------------------------------------------

using namespace pl_uartlite;

//-----------------------------------------------------------------------------

static volatile int exit_flag = 0;
void local_signal_handler(int signo)
{
    exit_flag = 1; 
	fprintf(stderr, "\n");
}

//-----------------------------------------------------------------------------

void write_job_wrapper(pl_uart& uart)
{
    uart.write_thread();
}

//-----------------------------------------------------------------------------

void read_job_wrapper(pl_uart& uart)
{
    uart.read_thread();
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // Базовый адрес и размер PL UART на шине AXI
    uint32_t base_address = get_from_cmdline<uint32_t>(argc, argv, "-b", 0x42C00000);
    uint32_t aperture_size = 0x10000;

    // выход по Ctrl+C
    signal(SIGINT, local_signal_handler);

    std::deque<uint8_t> rd_queue;
    std::mutex rd_lock;
    std::deque<uint8_t> wr_queue;
    std::mutex wr_lock;

    // PL UARTLITE UNIT
    pl_uart uart(base_address, aperture_size, rd_queue, rd_lock, wr_queue, wr_lock);

    fprintf(stderr, "Press enter to start UART READ/WRITE THREADS...\n");
    getchar();

    auto job_write = make_job<std::thread>(write_job_wrapper, std::ref(uart));
	auto job_read = make_job<std::thread>(read_job_wrapper, std::ref(uart));

	while (!exit_flag) {
		if(rd_queue.empty()) {
			ipc_delay(20);
		} else {
    		std::lock_guard<std::mutex> _rlock(rd_lock);
			std::lock_guard<std::mutex> _wlock(wr_lock);
    		for (const auto& v : rd_queue) {
				// поместим символ из приемной очереди в очередь на передачу
				wr_queue.push_back(v);
				// удалим символ из приемной очереди
				rd_queue.pop_front();
				// напечатем принятый символ из приемной очереди
				fprintf(stderr, "%c", (int)v);
			}
		}
	}
/*
	// Оставил от предыдущей реализации примера. Новый вариант не проверялся.
	// Число элементов в UART (не используется в текущей реализации)
    const size_t N = get_from_cmdline<size_t>(argc, argv, "-n", 1);
	// Таймауцт приема/передачи UART (не используется в текущей реализации)
    const size_t timeout = get_from_cmdline<size_t>(argc, argv, "-t", 1000);
    fprintf(stderr, "Press enter to write data into WR_QUEUE... 1\n");
    getchar();
    {
        std::lock_guard<std::mutex> _wlock(wr_lock);
        for (int ii = 0x30; ii < 0x30 + N; ii++) {
            wr_queue.push_back(ii);
        }
    }
    ipc_delay(3000);
*/    
    uart.stop();

	job_write->join();
    job_read->join();

    return 0;
}
