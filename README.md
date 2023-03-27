# MQTTClient
使用正点原子mp157开发板实现mqtt远程控制led闪烁

## MQTTClient_message结构体
* MQTT客户端应用程序发布消息和接收消息都是围绕着这个结构体。MQTTClient_message数据结构描述了MQTT消息的负载和属性等相关信息，譬如消息的负载、负载的长度、qos、消息的保留标志、dup 标志等，但是消息主题不是这个结构体的一部分。该结构体内容如下：
```
typedef struct
{
    int payloadlen; //负载长度
    void* payload; //负载
    int qos; //消息的 qos 等级
    int retained; //消息的保留标志
    int dup; //dup 标志（重复标志）
    int msgid; //消息标识符，也就是前面说的 packetId
    ......
} MQTTClient_message;
```
* 当客户端发布消息时就需要实例化一个 MQTTClient_message对象，同理，当客户端接收到消息时，其实也就是接收到了MQTTClient_message对象。通常在实例化MQTTClient_message对象时会使用MQTTClient_message_initializer宏对其进行初始化。

## 创建一个客户端对象
* 在连接服务端之前，需要创建一个客户端对象，使用MQTTClient_create函数创建：
```
int MQTTClient_create(MQTTClient *handle,
    const char *serverURI,
    const char *clientId,
    int persistence_type,
    void *persistence_context 
);
```
__handle__：MQTT 客户端句柄；
__serverURL__：MQTT 服务器地址；
__clientId__：客户端 ID；
__persistence_type__：客户端使用的持久化类型：
* MQTTCLIENT_PERSISTENCE_NONE：使用内存持久性。如果运行客户端的设备或系统出现故障或关闭，则任何传输中消息的当前状态都会丢失，并且即使在QoS1和QoS2下也可能无法传递某些消息。
* MQTTCLIENT_PERSISTENCE_DEFAULT：使用默认的（基于文件系统）持久性机制。传输中消息的状态保存在文件系统中，并在意外故障的情况下提供一些防止消息丢失的保护。
* MQTTCLIENT_PERSISTENCE_USER：使用特定于应用程序的持久性实现。使用这种类型的持久性可以控制应用程序的持久性机制。应用程序必须实现MQTTClient_persistence接口。

__persistence_context__：如果使用MQTTCLIENT_PERSISTENCE_NONE持久化类型，则该参数应设置为NULL。如果选择的是MQTTCLIENT_PERSISTENCE_DEFAULT持久化类型，则该参数应设置为持久化目录的位置，如果设置为 NULL，则持久化目录就是客户端应用程序的工作目录。
__返回值__：客户端对象创建成功返回MQTTCLIENT_SUCCESS，失败将返回一个错误码。
__使用示例__
```
MQTTClient client;
int rc;
/* 创建 mqtt 客户端对象 */
if (MQTTCLIENT_SUCCESS !=
        (rc = MQTTClient_create(&client, "tcp://iot.ranye-iot.net:1883",
        "dt_mqtt_2_id",
        MQTTCLIENT_PERSISTENCE_NONE, NULL))) {
    printf("Failed to create client, return code %d\n", rc);
    return EXIT_FAILURE;
}
```
* 注意，"tcp://iot.ranye-iot.net:1883"地址中，第一个冒号前面的tcp表示我们使用的是TCP连接；后面的1883表示MQTT服务器对应的端口号。

## 连接服务器
* 客户端创建之后，便可以连接服务器了，调用MQTTClient_connect函数连接：
```
int MQTTClient_connect(MQTTClient handle,
    MQTTClient_connectOptions *options
);
```
__handle__：客户端句柄；
__options__：一个指针。指向一个MQTTClient_connectOptions结构体对象MQTTClient_connectOptions结构体中包含了keepAlive、cleanSession以及一个指向MQTTClient_willOptions结构体对象的指针will_opts；MQTTClient_willOptions结构体包含了客户端遗嘱相关的信息，遗嘱主题、遗嘱内容、遗嘱消息的 QoS 等级、遗嘱消息的保留标志等。
__返回值__：连接成功返回MQTTCLIENT_SUCCESS，是否返回错误码：
* 1：连接被拒绝。不可接受的协议版本，不支持客户端的MQTT协议版本
* 2：连接被拒绝：标识符被拒绝
* 3：连接被拒绝：服务器不可用
* 4：连接被拒绝：用户名或密码错误
* 5：连接被拒绝：未授权
* 6-255：保留以备将来使用
```
typedef struct
{
    int keepAliveInterval; //keepAlive
    int cleansession; //cleanSession
    MQTTClient_willOptions *will; //遗嘱相关
    const char *username; //用户名
    const char *password; //密码
    int reliable; //控制同步发布消息还是异步发布消息
    ......
    ......
} MQTTClient_connectOptions;
```
```
typedef struct
{
    const char *topicName; //遗嘱主题
    const char *message; //遗嘱内容
    int retained; //遗嘱消息的保留标志
    int qos; //遗嘱消息的 QoS 等级
    ......
    ......
} MQTTClient_willOptions;
```
__使用示例__：
```
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;

    ......

 /* 连接服务器 */
 will_opts.topicName = "dt2914/willTopic"; //遗嘱主题
 will_opts.message = "Abnormally dropped"; //遗嘱内容
 will_opts.retained = 1; //遗嘱保留消息
 will_opts.qos = 0; //遗嘱 QoS 等级
 conn_opts.will = &will_opts;
 conn_opts.keepAliveInterval = 30; //客户端 keepAlive 间隔时间
 conn_opts.cleansession = 0; //客户端 cleanSession 标志
 conn_opts.username = "dt_mqtt_2"; //用户名
 conn_opts.password = "dt291444"; //密码
 if (MQTTCLIENT_SUCCESS !=
        (rc = MQTTClient_connect(client, &conn_opts))) {
    printf("Failed to connect, return code %d\n", rc);
    return EXIT_FAILURE;
 }
```
* 通常在定义MQTTClient_connectOptions对象时会使用MQTTClient_connectOptions_initializer宏对其进行初始化操作；而在定义MQTTClient_willOptions对象时使用MQTTClient_willOptions_initializer宏对其初始化。

## 设置回调函数
* 调用MQTTClient_setCallbacks函数为应用程序设置回调函数，MQTTClient_setCallbacks可设置多个回调函数，包括：断开连接时的回调函数cl（当客户端检测到自己掉线时会执行该函数，如果将其设置为NULL表示应用程序不处理断线的情况）、接收消息的回调函数ma（当客户端接收到服务端发送过来的消息时执行该函数，必须设置此函数否则客户端无法接收消息）、发布消息的回调函数dc（当客户端发布的消息已经确认发送时执行该回调函数，如果你的应用程序采用同步方式发布消息或者您不想检查是否成功发送时，您可以将此设置为 NULL）。
```
int MQTTClient_setCallbacks(MQTTClient handle,
    void *context,
    MQTTClient_connectionLost *cl,
    MQTTClient_messageArrived *ma,
    MQTTClient_deliveryComplete *dc
);
```
__handle__：客户端句柄；
__context__：执行回调函数的时候，会将context参数传递给回调函数，因为每一个回调函数都设置了一个参数用来接收context参数。
__cl__：一个MQTTClient_connectionLost类型的函数指针，如下：
```
typedef void MQTTClient_connectionLost(void *context, char *cause);
```
* 参数cause表示断线的原因，是一个字符串。

__ma__：一个MQTTClient_messageArrived类型的函数指针，如下：
```
typedef int MQTTClient_messageArrived(void *context, char *topicName,
    int topicLen, MQTTClient_message *message);
```
* 参数topicName表示消息的主题名，topicLen表示主题名的长度；参数message指向一个MQTTClient_message对象，也就是客户端所接收到的消息。

__dc__：一个MQTTClient_deliveryComplete类型的函数指针，如下：
```
typedef void MQTTClient_deliveryComplete(void* context, MQTTClient_deliveryToken dt);
```
* 参数dt表示MQTT消息的值，将其称为传递令牌。发布消息时（应用程序通过MQTTClient_publishMessage函数发布消息），MQTT协议会返回给客户端应用程序一个传递令牌；应用程序可以通过将调用MQTTClient_publishMessage()返回的传递令牌与传递给此回调的令牌进行匹配来检查消息是否已成功发布。
* 前面提到了“同步发布消息”这个概念，既然有同步发布，那必然有异步发布，确实如何！那如何控制是同步发布还是异步发布呢？就是通过MQTTClient_connectOptions对象中的reliable成员控制的，这是一个布尔值，当reliable=1时使用同步方式发布消息，意味着必须完成当前正在发布的消息（收到确认）之后才能发布另一个消息；如果reliable=0则使用异步方式发布消息。
* 当使用MQTTClient_connectOptions_initializer宏对MQTTClient_connectOptions对象进行初始化时，reliable标志被初始化为 1，所以默认是使用了同步方式。
__返回值__：成功返回MQTTCLIENT_SUCCESS，失败返回MQTTCLIENT_FAILURE。
__注意：调用MQTTClient_setCallbacks函数设置回调必须在连接服务器之前完成！__
__使用示例__:
```
static void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
}

static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("topic: %s\n", topicName);
    printf("message: <%d>%s\n", message->payloadlen, (char *)message->payload);
    MQTTClient_freeMessage(&message); //释放内存
    MQTTClient_free(topicName); //释放内存
    return 1;
}

static void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf(" cause: %s\n", cause);
}

int main(void)
{
    ......

    /* 设置回调 */
    if (MQTTCLIENT_SUCCESS !=
    (rc = MQTTClient_setCallbacks(client, NULL, connlost,
    msgarrvd, delivered))) {
    printf("Failed to set callbacks, return code %d\n", rc);
    return EXIT_FAILURE;
    }

    ......
}
```
对于 msgarrvd 函数有两个点需要注意：
* 退出函数之前需要释放消息的内存空间，必须调用MQTTClient_freeMessage函数；同时也要释放主题名称占用的内存空间，必须调用MQTTClient_free。
* 函数的返回值。此函数的返回值必须是0或1，返回1表示消息已经成功处理；返回0则表示消息处理存在问题，在这种情况下，客户端库将重新调用MQTTClient_messageArrived()以尝试再次将消息传递给客户端应用程序，所以返回0时不要释放消息和主题所占用的内存空间，否则重新投递失败。

## 发布消息
* 当客户端成功连接到服务端之后，便可以发布消息或订阅主题了，应用程序通过MQTTClient_publishMessage库函数来发布一个消息：
```
int MQTTClient_publishMessage(MQTTClient handle,
    const char *topicName,
    MQTTClient_message *msg,
    MQTTClient_deliveryToken *dt 
);
```
__handle__：客户端句柄；
__topicName__：主题名称。向该主题发布消息。
__msg__：指向一个MQTTClient_message对象的指针。
__dt__：返回给应用程序的传递令牌。
__返回值__：成功返回 MQTTCLIENT_SUCCESS，失败返回错误码。
__使用示例__
```
 MQTTClient_message pubmsg = MQTTClient_message_initializer;
 MQTTClient_deliveryToken token;
 ......
 /* 发布消息 */
 pubmsg.payload = "online"; //消息内容
 pubmsg.payloadlen = 6; //消息的长度
 pubmsg.qos = 0; //QoS 等级
 pubmsg.retained = 1; //消息的保留标志
 if (MQTTCLIENT_SUCCESS !=
        (rc = MQTTClient_publishMessage(client, "dt2914/testTopic", &pubmsg, &token))) {
    printf("Failed to publish message, return code %d\n", rc);
    return EXIT_FAILURE;
 }
```
## 订阅主题和取消订阅主题
* 客户端应用程序调用MQTTClient_subscribe函数来订阅主题：
```
int MQTTClient_subscribe(MQTTClient handle,
    const char *topic,
    int qos
);
```
__handle__：客户端句柄；
__topic__：主题名称。客户端订阅的主题。
__qos__：QoS 等级。
__返回值__：成功返回 MQTTCLIENT_SUCCESS，失败返回错误码。
__使用示例__
```
 ......

 if (MQTTCLIENT_SUCCESS !=
        (rc = MQTTClient_subscribe(client, "dt2914/testTopic", 0))) {
    printf("Failed to subscribe, return code %d\n", rc);
    return EXIT_FAILURE;
 }
 
 ......
```
* 当客户端想取消之前订阅的主题时，可调用MQTTClient_unsubscribe函数，如下所示：
```
int MQTTClient_unsubscribe(MQTTClient handle,
    const char *topic
);
```
__handle__：客户端句柄；
__topic__：主题名称。取消订阅该主题。
返回值：成功返回 MQTTCLIENT_SUCCESS，失败返回错误码。
## 断开服务端连接
* 当客户端需要主动断开与客户端连接时，可调用MQTTClient_disconnect函数：
```
int MQTTClient_disconnect(MQTTClient handle,
    int timeout 
);
```
__handle__：客户端句柄；
__timeout__：超时时间。客户端将断开连接延迟最多timeout时间（以毫秒为单位），以便完成正在进行中的消息传输。
__返回值__：如果客户端成功从服务器断开连接，则返回MQTTCLIENT_SUCCESS；如果客户端无法与服务器断开连接，则返回错误代码。
