#!/bin/bash

watch -n 0.1 'uptime; micsmc -c mic0 | grep "Device Utilization\|Core #"'
