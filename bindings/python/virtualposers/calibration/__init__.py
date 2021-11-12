# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

"""Calibration functions."""
from ..logging import log

logger = log.setup_custom_logger(
    name=__name__, level="INFO", file="../logs/calibration.log", console_logging=False
)

logger.debug("created calibration.log log file")
