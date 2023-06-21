@pytest.mark.parametrize("policy, threads, num_clients, queue_size, times, dynamic_max, files",
                         [
                             ("dynamic", 8, 15, 16, 20, 25, LOCKS_FILES),
                             ("dynamic", 32, 4, 64, 10, 15, LOCKS2_FILES),
                             ("dynamic", 64, 5, 128, 6, 8, LOCKS3_FILES),
                             ("dynamic", 25, 20, 27, 25, 30, LOCKS4_FILES),
                         ])
def test_locks(policy, threads, num_clients, queue_size, times, dynamic_max, files, server_port):
    with Server("./server", server_port, threads, queue_size, policy, dynamic_max) as server:
        sleep(0.1)
        for _ in range(times):
            for file_name, options in files.items():
                clients = []
                for _ in range(num_clients):
                    session = FuturesSession()
                    clients.append((session, session.get(f"http://localhost:{server_port}/{file_name}")))
                for client in clients:
                    response = client[1].result()
                    client[0].close()
                    expected = options[1]
                    expected_headers = options[2]
                    if options[0]:
                        validate_response_full(response, expected_headers, expected)
                    else:
                        validate_response_binary(response, expected_headers, expected)


class Server:
    def __init__(self, path, port, threads, queue_size, policy, dynamic_max):
        self.path = str(path)
        self.port = str(port)
        self.threads = str(threads)
        self.queue_size = str(queue_size)
        self.policy = str(policy)
        self.dynamic_max = str(dynamic_max)