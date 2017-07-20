FROM python:3

EXPOSE 8002

WORKDIR /usr/src/freeciv-proxy
COPY . .
RUN pip install --no-cache-dir -r requirements.txt

CMD [ "python", "./freeciv-proxy.py" ]
