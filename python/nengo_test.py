import ev3_nengo
import nengo
import numpy as np

with nengo.Network() as model:
    node_in = nengo.Node(np.zeros(4))
    node_lego = nengo.Node(ev3_nengo.make_node_fun('ev3_1', exe="./ev3_broker_client"), size_in=4, size_out=4)

    nengo.Connection(node_in, node_lego)