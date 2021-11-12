# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) 2020 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>
# Copyright (C) 2020 saahasam

"""Headset and controller tracers."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/trackers.log"
)

logger.debug("created trackers.log log file")
