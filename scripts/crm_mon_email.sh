#! /bin/sh

# This is a helper script intended to be used with the ClusterMon RA. It
# uses environment variables from the crm_mon binary; the original script is
# based on this SNMP helper script from Florian Crouzat:
# https://github.com/ClusterLabs/pacemaker/blob/master/extra/pcmk_snmp_helper.sh
#
# Example resource agent (ocf:pacemaker:ClusterMon) configuration:
# primitive p_notify ocf:pacemaker:ClusterMon \
#     params user="root" update="30" \
#     extra_options="-E /usr/local/bin/crm_mon_email.sh -e root" \
#     op monitor on-fail="restart" interval="10"
# clone clone_notify p_notify \
#     meta target-role="Started"

# Test for a failed monitor operation, or anything that isn't monitor
if [ "x${CRM_notify_rc}" != "x0" -a "x${CRM_notify_task}" = "xmonitor" ] || \
	[ "x${CRM_notify_task}" != "xmonitor" ] && \
	[ -n "${CRM_notify_recipient}" ]; then
	# Send a notification email
	sendmail -t <<-EOF
	To: ${CRM_notify_recipient}
	From: root
	Subject: ESOS Cluster Status Change - `hostname` (`date`)

	CRM_notify_node: ${CRM_notify_node}
	CRM_notify_rsc: ${CRM_notify_rsc}
	CRM_notify_task: ${CRM_notify_task}
	CRM_notify_desc: ${CRM_notify_desc}
	CRM_notify_rc: ${CRM_notify_rc}
	CRM_notify_target_rc: ${CRM_notify_target_rc}
	CRM_notify_status: ${CRM_notify_status}
	EOF
fi

