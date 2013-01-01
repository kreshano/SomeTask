package MyJms;

import javax.jms.*;

import org.apache.activemq.ActiveMQConnectionFactory;
import org.apache.log4j.PropertyConfigurator;


public class JmsBase {

    protected String url;
    protected String subject;
    protected boolean isTopic;
    protected boolean isQueue;
    protected JmsConsumerException consumerException;
    protected Connection connection;
    
    public JmsBase(String jmslUrl) throws JmsConsumerException
    {
        url = "failover://tcp://" + jmslUrl;
        PropertyConfigurator.configure("conf/log4j.properties");
        ActiveMQConnectionFactory connectionFactory = new ActiveMQConnectionFactory(url);
        try {
            connection = connectionFactory.createConnection();
        } catch (JMSException e) {
            // TODO Auto-generated catch block
            consumerException = new JmsConsumerException(e);
            throw consumerException;
        }
        
        isQueue = false;
        isTopic = false;
        subject = "";
    }
    
    /**
     * TO set the Queue to be listen this setQueue method will used
     * 
     * @param queueName Name of the Queue to which the consumer will listen
     */
    public void setQueue(String queueName)
    {
        isQueue = true;
        isTopic = false;
        subject = queueName;
    }
    
    /**
     * TO set the Topic to be listen this setTopic method will used
     * 
     * @param topicName Name of the Topic to which the consumer will listen
     */
    public void setTopic(String topicName)
    {
        isQueue = false;
        isTopic = true;
        subject = topicName;
    }    
    
    /**
     * 
     * @return Return the location of running JMS to which the consumer is listen
     */
    public String getJmsLocation()
    {
        return url;
    }
    
    /**
     * 
     * @return Return the current set Queue name
     */
    public String getQueue()
    {
        if ( isQueue )
            return subject;
        else
            return "";
    }
    
    /**
     * 
     * @return Return the current set Topic name
     */
    public String getTopic()
    {
        if ( isTopic )
            return subject;
        else
            return "";
    }
    
    public Connection getConnection()
    {
        return connection;
    }
}


package MyJms;

import javax.jms.*;


/**
 * JmsConsumer class will use to listen and consume channel messages.
 * Here's one sample use of this class
 * <pre>
 * JmsConsumer consumer = new JmsConsumer("host:port"); //New consumer with location
 * consumer.setTopic("topic.Name"); //set the topic Name
 * consumer.setListener( new MyHandler() ); //set handler to consume message
 * consumer.startListen(); //start listening
 * </pre>
 * 
 * @author kreshano
 *
 *
 */

public class JmsConsumer extends JmsBase {

    private Session session;
    private Destination destination;
    private MessageConsumer consumer;
    
    /**
     * Will create a consumer connection to the given jmsUrl location.
     * The jmsUrl must be a valid location[like tcp://host:port]
     *  
     * 
     * @param jmslUrl Location of the running JMS service.
     * @throws JmsConsumerException
     */
    public JmsConsumer(String jmslUrl) throws JmsConsumerException
    {
        super(jmslUrl);
    }

    /**
     * 
     * @param messageListener The listener class which will consume the message
     * @throws JmsConsumerException
     */
    public void setListener(MessageListener messageListener) throws JmsConsumerException
    {
        if( !(isTopic || isQueue) ) {
            throw new JmsConsumerException("Set Queue or Topic first");
        }        
        try {
            session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            if( isTopic  )
                destination = session.createTopic(subject);
            else
                destination = session.createQueue(subject);
            consumer = session.createConsumer(destination);
            consumer.setMessageListener(messageListener);
        }
        catch (JMSException e) {            
            consumerException =  new JmsConsumerException(e);
            throw consumerException;
        }
    }
    
    /**
     * 
     * @throws JmsConsumerException
     */
    public void startListen() throws JmsConsumerException
    {
        try {
            connection.start();
            
        } catch (JMSException e) {
            consumerException =  new JmsConsumerException(e);
            throw consumerException;
        }
    }
}
