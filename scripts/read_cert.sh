#! /bin/sh

# Helper script used by ESOS Web UI which simply prints the ESOS RPC Agent
# PEM file (X.509 certificate + private key) to the screen.

cat /etc/stunnel/rpc_agent.pem

