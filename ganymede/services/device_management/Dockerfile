# Would be great to get this running on Alpine. Needs investigation
FROM ubuntu:20.10 AS runtime

COPY bin/ganymede.services.device_management /opt/ganymede/ganymede.services.device_management

ENV PORT=3000
EXPOSE ${PORT}
ENTRYPOINT ["/opt/ganymede/ganymede.services.device_management"]