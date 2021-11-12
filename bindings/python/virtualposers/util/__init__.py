# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) 2020 sahasam
# Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

"""Virtualreality Utility Module"""
from ..logging import log

logger = log.setup_custom_logger(name=__name__, level="INFO", file="../logs/util.log")

logger.debug("created util.log log file")
