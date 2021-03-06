function consumer
    % A consumer can attend requests from a producer and send the result to a collector.
    %
    % To receive work, it uses a PULL socket, and to send the result it uses a PUSH socket.
    %
    % Example borrowed from
    % http://learning-0mq-with-pyzmq.readthedocs.org/en/latest/pyzmq/patterns/pushpull.html

    id = randi([1, 10005], 1, 1);
    fprintf('I am consumer #%d', id);

    context = zmq_ctx_new();
    % recieve work
    rx  = zmq_socket(context, 'ZMQ_PULL');
    producerAddress = 'tcp://127.0.0.1:5557';
    zmq_connect(rx, producerAddress);
    % send results
    tx = zmq_socket(context, 'ZMQ_PUSH');
    collectorAddress = 'tcp://127.0.0.1:5558';
    zmq_connect(tx, collectorAddress);

    while (1)
        data = str2double(char(zmq_recv(rx)));
        if (mod(data, 2) == 0)
            result = sprintf('%d %d', id, data);
            zmq_send(tx, uint8(result));
        end
    end
end